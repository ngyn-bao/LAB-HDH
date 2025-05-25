#!/bin/bash

BKUDIR='.bku'
TRACKEDFILES='.bku/tracked_file'
HISTORY='.bku/history.log'
COMMITSDIR='.bku/commits'

check(){
    if [ ! -d "$BKUDIR" ]; then
	    echo 'Must be a BKU root folder.'
	    exit 1
    fi
}


init(){
	if [ -d "$BKUDIR" ]; then 
		echo 'Error: Backup already initialized in this folder.'
		exit 1
	fi

	mkdir -p "$BKUDIR" "$COMMITSDIR" || { echo "Failed to create backup directories."; exit 1; }
	touch "$HISTORY" "$TRACKEDFILES"|| { echo "Failed to create backup history file."; exit 1; }
	echo 'Backup initialized.'
	log_action "$(date +"%H:%M-%d/%m/%Y"): BKU Init."
}

add(){
    check
	
	if [ $# -eq 0 ]; then
        files=()
        while IFS= read -r file; do
            files+=("${file#./}") 
        done < <(find . -type f ! -path "./$BKUDIR/*")
    else 
        files=("$@")
    fi 

	for file in "${files[@]}"; do
		if [ ! -f "$file" ]; then
			echo Error: "$file" does not exist.
			continue
		elif ! grep -qx "$file" $TRACKEDFILES; then
			echo "$file" >> $TRACKEDFILES
			echo Added "$file" to backup tracking.
		else
			echo "Error: $file is already tracked."
		fi

		fileName=$(echo "$file" | tr '/' '_') 
		prevFile="$COMMITSDIR/latest-$fileName.tmp"
		cp "$file" "$prevFile"
	done
}


status(){
    check
	if [ ! -s $TRACKEDFILES ];  then
		echo "Error: Nothing has been tracked."
		return
	fi

	if [ $# -eq 0 ]; then
		mapfile -t files < "$TRACKEDFILES"
	else
		files=( "$@" )
	fi

	failed=0

	for file in "${files[@]}"; do
		if ! grep -qx "$file" "$TRACKEDFILES"; then
			echo Error: "$file" is not tracked.
			((failed++))
			continue
		fi

		lastCommitFile="$COMMITSDIR/latest-$(echo "$file" | tr '/' '_').tmp"

		if [ -z "$lastCommitFile" ]; then
			echo Error: "$file: No previous commit found."
			continue
		fi

		diffOutput=$(diff -u "$lastCommitFile" "$file")
		if [ -n "$diffOutput" ]; then
			echo "$file:"
			echo "$diffOutput"
		else
			echo "$file: No changes."
		fi 
	done

	if [ "$failed" -eq "${#files[@]}" ]; then
			echo ${#files[@]} is not tracked.
			exit 1
		fi
}

commit(){
	check
	if [ -z "$1" ]; then
		echo 'Error: Commit message is required.'
		exit 1
	fi

	commitMessage=$1
	shift
	commitId=$(date +"%H:%M-%d/%m/%Y")

 	if [ ! -s $TRACKEDFILES ];  then
		echo 'Error: No change to commit.'
		return
	fi

	if [ $# -eq 0 ]; then
		mapfile -t files < "$TRACKEDFILES"
	else
		files=("$@")
	fi
	changedFiles=""
	changesMade=false

	for file in "${files[@]}"; do
		if ! grep -qx "$file" "$TRACKEDFILES"; then
			echo "$file" is not tracked.
			continue 
		fi	
		
		fileName=$(echo "$file" | tr '/' '_') 
		commitIdName=$(date +"%H-%M-%d_%m_%Y")
		commitFile="$COMMITSDIR/$commitIdName-$fileName.diff"	
		prevFile="$COMMITSDIR/latest-$fileName.tmp"

		if [ ! -f "$prevFile" ]; then
            diffOutput=$(cat "$file")
        else
            diffOutput=$(diff "$prevFile" "$file")
        fi

		if [ -n "$diffOutput" ]; then
            echo "$diffOutput" > "$commitFile"
			cp "$file" "$prevFile"
            echo "Committed $file with ID $commitId."
            changedFiles+="$file"
            changesMade=true
        fi
	done

	if [ "$changesMade" = false ]; then
		echo 'Error: No change to commit.'
		exit 1
	fi
	
	changedFilesString=$(IFS=,; echo "${changedFiles[*]}")
    log_action "$commitId: $commitMessage ($changedFilesString)." 
}	

history(){
	check
	if [ ! -s $HISTORY ]; then
		echo No commit history found.
		exit 1
	fi
	cat $HISTORY
}

restore(){
	check

	if [ ! -s "$TRACKEDFILES" ]; then
		echo "No file to be restored."
		exit 1
	fi

	if [ $# -eq 0 ]; then
		mapfile -t files < "$TRACKEDFILES"
	else
		files=( "$@" )
	fi

	changesFound=false

	for file in "${files[@]}"; do
		if ! grep -qx "$file" "$TRACKEDFILES"; then
			echo $file is not tracked.
			continue 
		fi	

		fileName=$(echo "$file" | tr '/' '_') 
		diffOutput=$(find "$COMMITSDIR" -type f -name "*-$fileName.diff" | sort | tail -n 1)

		if [ -z "$diffOutput" ]; then
			echo Error: "No previous version available for $file"
			continue
		else
			patch -R "$file" < "$diffOutput"
			echo "Restored $file to its previous version."
			commit "Restored to previous version"
			changesFound=true
		fi
	done

	if [ "$changesFound" = false ]; then
		echo 'Error: No file to be restored.'
		exit 1
	fi
}

schedule(){
	check

	case "$1" in
		--daily) 
		(crontab -l 2>/dev/null | grep -v "bku.sh commit" ; echo "0 0 * * * bku.sh commit \"Scheduled backup\"") | crontab -
		echo "Scheduled daily backups at daily."
		;;
		--hourly)
		(crontab -l 2>/dev/null | grep -v "bku.sh commit" ; echo "0 * * * * bku.sh commit \"Scheduled backup\"") | crontab -
		echo "Scheduled hourly backups."
		;;
		--weekly)
		(crontab -l 2>/dev/null | grep -v "bku.sh commit" ; echo "0 0 * * 1 bku.sh commit \"Scheduled backup\"") | crontab -
		echo "Scheduled weekly backups."
		;;
		--off)
		crontab -l 2>/dev/null | grep -v "bku.sh commit" | crontab -
		echo "Backup scheduling disabled."
		;;
		*)
		echo "Usage: bku schedule {--daily|--hourly|--weekly|--off}"
		;;
	esac
}



stop(){
	if [ ! -d "$BKUDIR" ]; then
		echo 'Error: No backup system to be removed.'
		exit 1
	fi
	rm -rf "$BKUDIR"
	crontab -l | grep -v "$(realpath "$0") commit" | crontab - 
	echo "Backup system removed."
}

log_action() {
    local entry="$1"
    local stack=()

    if [ -f "$HISTORY" ]; then
        while IFS= read -r line; do
            stack+=("$line")
        done < "$HISTORY"
    fi

    echo "$entry" > "$HISTORY"

    while [ ${#stack[@]} -gt 0 ]; do
        echo "${stack[0]}" >> "$HISTORY"
        stack=("${stack[@]:1}") 
    done
}

case "$1" in
    init) init ;;
    add) shift; add "$@" ;;
    status) shift; status "$@" ;;
    commit) shift; commit "$@" ;;
    history) history ;;
    restore) shift; restore "$@" ;;
    schedule) shift; schedule "$@" ;;
    stop) stop ;;
    *) echo "Usage: bku {init|add|status|commit|history|restore|schedule|stop|install}" ;;
esac
