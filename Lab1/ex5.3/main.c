#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#if __has_include(<regex.h>)
#include <regex.h>
#define USE_REGEX 1
#else
#define USE_REGEX 0
#endif

#define BUFFER_SIZE 256

void helpDisplay();
int isValidProjectName(const char* name);
void createFile(const char* path, const char* content);
int createProject(const char* name, int withGit);
int checkGitInstalled();
int verifyBuild(const char* name);

int main(int argc, char* argv[]) {
    char* projectName = NULL;
    int withGit = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--name") == 0) {
            if (i + 1 < argc)
                projectName = argv[++i];
            else {
                fprintf(stderr, "Error: Missing project name after '--name'.\n");
                exit(1);
            }
        }
        else if (strcmp(argv[i], "--with-git") == 0)
            withGit = 1;
        else if (strcmp(argv[i], "--help") == 0)
            helpDisplay();
        else {
            fprintf(stderr, "Error: Unknown argument '%s'\n Use 'cnew --help' to see help message detailing available options", argv[i]);
            exit(1);
        }
    }

    if (projectName == NULL) {
        fprintf(stderr, "Error: Missing required '--name' option.\n");
        exit(1);
    }

    if (isValidProjectName(projectName) != 0) {
        fprintf(stderr, "Error: Project name '%s' is invalid. Use only letters, numbers, and hyphens (e.g., my-project).\n", projectName);
        exit(1);
    }

    if (createProject(projectName, withGit) != 0) {
        fprintf(stderr, "Failed to create project '%s'.", projectName);
        exit(1);
    }

    if (verifyBuild(projectName) != 0) {
        fprintf(stderr, "Failed to verify project '%s' build.", projectName);
        exit(1);
    }

    return 0;
}

void helpDisplay() {
    printf("Usage: cnew [options]\n");
    printf("Options:\n");
    printf("  --name <project-name>    Create a new C project with the given name (required).\n");
    printf("                           Name must contain only letters, numbers, and hyphens.\n");
    printf("  --with-git               Initialize the project as a Git repository with a .gitignore.\n");
    printf("  --help                   Display this help message.\n");
    printf("Example:\n");
    printf("  cnew --name my-project --with-git\n");
    exit(0);
}

int isValidProjectName(const char* name) {
#if USE_REGEX == 1
    regex_t regex;
    int ret = regcomp(&regex, "^[a-zA-Z0-9-]+$", REG_EXTENDED);
    if (ret) {
        fprintf(stderr, "Error: Failed to compile regex.\n");
        return 1;
    }
    ret = regexec(&regex, name, 0, NULL, 0);
    regfree(&regex);
    if (ret == 0)  return 0;
#else
    for (int i = 0; name[i] != '\0'; i++) {
        if (!(('a' <= name[i] && name[i] <= 'z') ||
            ('A' <= name[i] && name[i] <= 'Z') ||
            ('0' <= name[i] && name[i] <= '9') ||
            (name[i] == '-')))
            return 1;
    }
    return 0;
#endif
    return -1;
}

void createFile(const char* path, const char* content) {
    FILE* file = fopen(path, "w");
    if (file == NULL) {
        fprintf(stderr, "Error creating file '%s'", path);
        exit(1);
    }
    fprintf(file, "%s", content);
    fclose(file);
}

int createProject(const char* name, int withGit) {
    char path[BUFFER_SIZE];

    if (access(name, F_OK) == 0) {
        fprintf(stderr, "Error: Directory '%s' already exists.\n", name);
        return 1;
    }

    mkdir(name, 0777);
    snprintf(path, BUFFER_SIZE, "%s/src", name);
    mkdir(path, 0777);
    snprintf(path, BUFFER_SIZE, "%s/include", name);
    mkdir(path, 0777);

    snprintf(path, BUFFER_SIZE, "%s/src/main.c", name);
    createFile(path, "#include <stdio.h>\n\n"
        "int main() {\n"
        "    printf(\"Hello, World!\\n\");\n"
        "    return 0;\n"
        "}\n");

    snprintf(path, BUFFER_SIZE, "%s/README.md", name);
    char* readmeContent = malloc(strlen(name) + 3);
    if (!readmeContent) {
        perror("Error: Memory allocation failed");
        return 1;
    }
    strcpy(readmeContent, "# ");
    strcat(readmeContent, name);
    createFile(path, readmeContent);
    free(readmeContent);

    snprintf(path, BUFFER_SIZE, "%s/Makefile", name);
    createFile(path, "CC = gcc\n\n"
        "SRC = src/main.c\n\n"
        "OUT = program\n\n"
        "all:\n\t$(CC) $(SRC) -o $(OUT)\n\n"
        "clean:\n\trm -f $(OUT) *.o\n");

    printf("Created C project '%s' with standard layout.\n", name);

    if (withGit == 1) {
        if (checkGitInstalled() != 0) {
            fprintf(stderr, "Error: Git is not installed. Install it to use --with-git.\n");
            return 1;
        }

        snprintf(path, BUFFER_SIZE, "cd %s && git init > /dev/null 2>&1", name);
        if (system(path) != 0) {
            fprintf(stderr, "Error: Failed to initialize Git repository.\n");
            return 1;
        }

        printf("Initialized Git repository.\n");

        snprintf(path, BUFFER_SIZE, "%s/.gitignore", name);
        createFile(path, "*.o\nprogram\n");
        printf("Added .gitignore for C projects.\n");
    }

    return 0;
}

int checkGitInstalled() {
    if (system("git --version > /dev/null 2>&1") == 0)
        return 0;
    return 1;
}

int verifyBuild(const char* name) {
    char command[BUFFER_SIZE];
    char output[BUFFER_SIZE];
    FILE* tempFile;

    snprintf(command, BUFFER_SIZE, "cd %s && make all > /dev/null 2>&1", name);
    if (system(command) != 0) {
        fprintf(stderr, "Error: Build project '%s' failed.\n", name);
        return 1;
    }

    snprintf(command, BUFFER_SIZE, "cd %s && ./program > output.txt 2>&1", name);
    if (system(command) != 0) {
        fprintf(stderr, "Error: Failed to execute ./program.\n");
        return 1;
    }

    snprintf(command, BUFFER_SIZE, "%s/output.txt", name);
    tempFile = fopen(command, "r");
    if (!tempFile) {
        fprintf(stderr, "Error: Could not open output file.\n");
        return 1;
    }

    if (fgets(output, sizeof(output), tempFile) == NULL) {
        fprintf(stderr, "Error: No output from ./program.\n");
        fclose(tempFile);
        return 1;
    }
    fclose(tempFile);

    snprintf(command, BUFFER_SIZE, "rm -f %s/output.txt", name);
    system(command);


    if (strcmp(output, "Hello, World!\n") != 0) {
        fprintf(stderr, "Error: Incorrect program output: '%s'\n", output);
        return 1;
    }

    printf("Running 'make all' to verify...\n");
    printf("Build successfully. Binary 'program' created.\n");
    printf("Output check successfully.\n");
    printf("Project setup complete.\n");

    return 0;
}
