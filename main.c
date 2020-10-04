#include "fat16.h"

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        printf("Brak podane argumentu\n");
        return 1;
    }

    int res = load_disc(argv[1]);
    if(res == 1)
    {
        printf("Blad funkcji fopen dla obrazu dysku\n");
        return 1;
    }

    res = load_fat();
    if(res == 1)
    {
        printf("Blad wczytywanie FAT\n");
        return 1;
    }

    printf("LISTA KOMEND\n");
    printf("|-------------------------------------------------------------------------------|\n");
    printf("exit - kończy działanie programu\n");
    printf("dir - wyświetla listę plików oraz katalogów w bieżącym katalogu obrazu wraz z informacją o ich wielkości oraz dacie modyfikacji\n");
    printf("cd <nazwa> – zmienia bieżący katalog obrazu na nazwa lub wyświetla komunikat o błędzie\n");
    printf("pwd - wyświetla nazwę aktualnego katalogu (ang. Print Working Directory\n");
    printf("cat <nazwa> – wyświetla zawartość pliku o nazwa z bieżącego katalogu obrazu\n");
    printf("get <nazwa> – zapisuje (kopiuje) plik o nazwie nazwa z bieżącego katalogu obrazu do bieżącego katalogu systemu operacyjnego\n");
    printf("zip <nazwa1> <nazwa2> <nazwa3> - otwiera pliki nazwa1 oraz nazwa2 a następnie wczytuje z nich ");
    printf("kolejne linie i zapisuje je naprzemiennie do pliku nazwa3 w bieżącym katalogu systemu operacyjnego\n");
    printf("rootinfo – wyświetla informacje o katalogu głównym woluminu\n");
    printf("spaceinfo – wyświetla informacje o kastrach\n");
    printf("fileinfo <nazwa> - wyświetla informacje o pliku nazwa z bieżącego katalogu obrazu\n");

    while(1)
    {
        char user_input[100];
        memset(user_input, 0, 100);

        fgets(user_input, 100, stdin);
        int len = strlen(user_input);
        user_input[len-1] = '\0';

        if(strcmp(user_input, "dir") == 0)
        {
            dir_function();
        }
        else if(strcmp(user_input, "pwd") == 0)
        {
            pwd_function();
        }
        else if(strcmp(user_input, "spaceinfo") == 0)
        {
            spaceinfo_function();
        }
        else if(strcmp(user_input, "rootinfo") == 0)
        {
            rootinfo_function();
        }
        else if(strcmp(user_input, "exit") == 0)
        {
            return 0;
        }
        else if(strcmp(user_input, "clear") == 0)
        {
            system("clear");
        }
        else
        {
            int pos;
            char type[70];
            memset(type, 0, 70);
            for(pos = 0; user_input[pos] != '\0'; pos++)
            {
                if(user_input[pos] == ' ')
                {
                    if(strcmp(type, "cd") == 0)
                    {
                        char filename[60];
                        memset(filename, 0, 60);
                        int x = 0;
                        for(int i = ++pos; user_input[i] != '\0'; i++)
                        {
                            filename[x++] = user_input[i];
                        }
                        cd_function(filename);
                        break;
                    }
                    else if(strcmp(type, "fileinfo") == 0)
                    {
                        char filename[60];
                        memset(filename, 0, 60);
                        int x = 0;
                        for(int i = ++pos; user_input[i] != '\0'; i++)
                        {
                            filename[x++] = user_input[i];
                        }
                        fileinfo_function(filename);
                        break;
                    }
                    else if(strcmp(type, "cat") == 0)
                    {
                        char filename[60];
                        memset(filename, 0, 60);
                        int x = 0;
                        for(int i = ++pos; user_input[i] != '\0'; i++)
                        {
                            filename[x++] = user_input[i];
                        }
                        cat_function(filename);
                        break;
                    }
                    else if(strcmp(type, "get") == 0)
                    {
                        char filename[60];
                        memset(filename, 0, 60);
                        int x = 0;
                        for(int i = ++pos; user_input[i] != '\0'; i++)
                        {
                            filename[x++] = user_input[i];
                        }
                        get_function(filename);
                        break;
                    }
                    else if(strcmp(type, "zip") == 0)
                    {
                        char filename1[60];
                        memset(filename1, 0, 60);
                        char filename2[60];
                        memset(filename2, 0, 60);
                        char filename3[60];
                        memset(filename3, 0, 60);
                        int x = 0;
                        int i = ++pos;
                        for(; user_input[i] != ' '; i++)
                        {
                            filename1[x++] = user_input[i];
                        }
                        x = 0;
                        i++;
                        for(; user_input[i] != ' '; i++)
                        {
                            filename2[x++] = user_input[i];
                        }
                        x = 0;
                        i++;
                        for(; user_input[i] != '\0'; i++)
                        {
                            filename3[x++] = user_input[i];
                        }
                        zip_function(filename1, filename2, filename3);
                        break;
                    }
                    else
                    {
                        printf("Zla komenda\n");
                        break;
                    }
                }
                type[pos] = user_input[pos];
            }
        } 
    }

    return 0;
}
