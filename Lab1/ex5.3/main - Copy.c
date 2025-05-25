//
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>

// Function to check if the project name is valid
int is_valid_project_name(const char *name)
{
    // Ensure that the project name only contains letters, numbers, and hyphens
    for (int i = 0; name[i] != '\0'; i++)
    {
        if (!isalnum(name[i]) && name[i] != '-')
        {
            return 0; // Invalid character found
        }
    }
    return 1; // Valid name
}
int is_git_installed()
{
    // Try running 'git --version' to check if Git is installed
    if (system("git --version > /dev/null 2>&1") == 0)
    {
        return 1; // Git is installed
    }
    return 0; // Git is not installed
}

// Function to create the project structure
int create_project_structure(const char *project_name, int with_git)
{
    char path[512]; // Increased buffer size for longer paths

    // Check if the project name is valid
    if (!is_valid_project_name(project_name))
    {
        printf("Error: Project name '%s' is invalid. Use only letters, numbers, and hyphens (e.g., my-project).\n", project_name);
        return -1;
    }

    // Check if the directory already exists
    snprintf(path, sizeof(path), "./%s", project_name);
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
    {
        printf("Error: Directory '%s' already exists.\n", project_name);
        return -1;
    }

    // Create project directory
    if (mkdir(path, 0755) != 0)
    {
        perror("Error creating project directory");
        return -1;
    }

    // Create src directory
    snprintf(path, sizeof(path), "./%s/src", project_name);
    if (mkdir(path, 0755) != 0)
    {
        perror("Error creating src directory");
        return -1;
    }

    // Create include directory
    snprintf(path, sizeof(path), "./%s/include", project_name);
    if (mkdir(path, 0755) != 0)
    {
        perror("Error creating include directory");
        return -1;
    }

    // Create Makefile in the project directory
    snprintf(path, sizeof(path), "./%s/Makefile", project_name);
    FILE *makefile = fopen(path, "w");
    if (makefile == NULL)
    {
        perror("Error creating Makefile");
        exit(1);
    }

    fprintf(makefile, "CC = gcc\n");
    fprintf(makefile, "CFLAGS = -Wall -Wextra -g\n");
    fprintf(makefile, "SRC = src/main.c\n");
    fprintf(makefile, "OBJ = $(SRC:.c=.o)\n");
    fprintf(makefile, "OUT = program\n");
    fprintf(makefile, "all: $(OBJ)\n");
    fprintf(makefile, "\t$(CC) $(CFLAGS) $(OBJ) -o $(OUT)\n");
    fprintf(makefile, "clean:\n");
    fprintf(makefile, "\trm -f $(OBJ) $(OUT)\n");
    fprintf(makefile, "install:\n");
    fprintf(makefile, "\tcp $(OUT) /usr/local/bin/cnew\n");
    fprintf(makefile, "uninstall:\n");
    fprintf(makefile, "\trm -f /usr/local/bin/cnew\n");
    fclose(makefile);

    // Create README.md in the project directory
    snprintf(path, sizeof(path), "./%s/README.md", project_name);
    FILE *readme = fopen(path, "w");
    if (readme == NULL)
    {
        perror("Error creating README.md");
        return -1;
    }
    fprintf(readme, "# %s\n", project_name);
    fprintf(readme, "This is a boilerplate C project created by cnew.\n");
    fclose(readme);

    // Create src/main.c in the src directory
    snprintf(path, sizeof(path), "./%s/src/main.c", project_name);
    FILE *main_c = fopen(path, "w");
    if (main_c == NULL)
    {
        perror("Error creating src/main.c");
        return -1;
    }
    fprintf(main_c, "#include <stdio.h>\n");
    fprintf(main_c, "int main() {\n");
    fprintf(main_c, "    printf(\"Hello, World!\\n\");\n");
    fprintf(main_c, "    return 0;\n");
    fprintf(main_c, "}\n");
    fclose(main_c);

    // Initialize Git repository if requested
    if (with_git)
    {
        if (!is_git_installed())
        {
            printf("Error: Git is not installed. Install it to use --with-git.\n");
            return -1;
        }

        // Initialize git repository
        snprintf(path, sizeof(path), "./%s", project_name);
        if (chdir(path) != 0)
        {
            perror("Error changing directory");
            return -1;
        }
        if (system("git init") != 0)
        {
            perror("Error initializing Git repository");
            return -1;
        }

        // Create .gitignore
        FILE *gitignore = fopen("./.gitignore", "w");
        if (gitignore == NULL)
        {
            perror("Error creating .gitignore");
            return -1;
        }
        fprintf(gitignore, "*.o\n");
        fprintf(gitignore, "program\n");
        fclose(gitignore);

        printf("Initialized Git repository.\n");
        printf("Added .gitignore for C projects.\n");

        // Change back to original directory
        if (chdir("..") != 0)
        {
            perror("Error changing back to original directory");
            return -1;
        }
    }

    printf("Created C project '%s' with standard layout.\n", project_name);
    return 0;
}

// Function to run the build process and check output
int verify_build(const char *project_name)
{
    char command[512];

    // Print message that 'make all' is running
    printf("Running 'make all' to verify...\n");

    // Run 'make all' to build the project
    snprintf(command, sizeof(command), "cd %s && make all", project_name);
    if (system(command) != 0)
    {
        printf("Error: Build failed.\n");
        return -1;
    }

    // Check if the binary 'program' was created
    snprintf(command, sizeof(command), "test -f %s/program", project_name);
    if (system(command) != 0)
    {
        printf("Error: Binary 'program' not created.\n");
        return -1;
    }

    // If the binary is created, print success message
    printf("Build successfully. Binary program created.\n");

    // Run the output check (execute the program)
    snprintf(command, sizeof(command), "cd %s && ./program", project_name);
    if (system(command) != 0)
    {
        printf("Error: Output check failed.\n");
        return -1;
    }

    // Print final success message
    printf("Output check successfully.\n");
    printf("Project setup complete.\n");

    return 0;
}
// Function to display help message
void print_help()
{
    printf("Usage: cnew [options]\n");
    printf("Options:\n");
    printf("--name <project-name>    Create a new C project with the given name (required).\n");
    printf("                         Name must contain only letters, numbers, and hyphens.\n");
    printf("--with-git               Initialize the project as a Git repository with a .gitignore.\n");
    printf("--help                   Display this help message.\n");
    printf("Example:\n");
    printf("cnew --name my-project --with-git\n");
}

// Main function
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        print_help();
        return 1;
    }

    char *project_name = malloc(1024);
    project_name[0] = '\0';
    int with_git = 0;
    // int count = 0;
    //  Parse command line arguments
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--name") == 0)
        {
            i++; // Move to the next argument, which is the project name
            while (i < argc && (argv[i][0] != '-' || (argv[i][0] == '-' && argv[i][1] != '-')))
            {
                strcat(project_name, argv[i]); // Concatenate the project name
                if (i + 1 < argc && argv[i + 1][0] != '-')
                {
                    strcat(project_name, " "); // Add a space between words
                }
                i++; // Move to the next argument
            }
        }
        else if (strcmp(argv[i], "--with-git") == 0)
        {
            with_git = 1;
        }
        else if (strcmp(argv[i], "--help") == 0)
        {
            print_help();
            return 0;
        }
        else
        {
            printf("Invalid option: %s\n", argv[i]);
            return 1;
        }
    }

    // If project name is passed, validate it
    if (project_name != NULL)
    {
        if (strchr(project_name, ' '))
        { // Check if the project name contains spaces
            printf("Error: Project name \"%s\" is invalid. Use only letters, numbers, and hyphens (e.g., my-project).\n", project_name);
            return 1;
        }

        if (!is_valid_project_name(project_name))
        {
            printf("Error: Project name \"%s\" is invalid. Use only letters, numbers, and hyphens (e.g., my-project).\n", project_name);
            return 1;
        }

        // printf("Project name: %s\n", project_name);
    }
    else
    {
        printf("Error: Project name is missing or invalid.\n");
        return 1;
    }

    if (project_name == NULL)
    {
        printf("Error: --name option is required.\n");
        print_help();
        return 1;
    }

    // Step 1: Create the project structure
    if (create_project_structure(project_name, with_git) != 0)
    {
        return 1; // Return error if project creation fails
    }

    // Step 2: Verify the build
    if (verify_build(project_name) != 0)
    {
        return 1; // Return error if build verification fails
    }
    free(project_name);
    return 0;
}
