#include "fat16.h"

#define SECTOR_SIZE 512

struct SUPPORT_STRUCT Fat_struct;

//Odczytuje sektor z wczytanego dysku który znajduje sie pod uchwytem
size_t readblock(void* buffer, uint32_t first_block, size_t block_count)
{
    //Sprawdza czy wszystkie argumenty sa poprawne
    //Jesli nie zwraca 0
	if(Fat_struct.disc_handle == NULL || buffer == NULL || block_count == 0)
    {
        return 0;
    } 
		
    //Ustaiwienie pozycji wskaźnika w dysku, na elementcie podanym jako drugi argument,
    //Liczone od początku pliki
	int check = fseek(Fat_struct.disc_handle, first_block * SECTOR_SIZE, SEEK_SET);
	
    //Jeśli coś poszło nie tak funkcja zwraca 0
    if(check != 0)
    {
        return 0;
    }

    //Odczytanie do podanego bufora odpowiedniej ilości bloków o rozmiarze SECTOR_SIZE,
    //Z dysku który znajduje sie w uchwycie
	size_t count = fread(buffer, SECTOR_SIZE, block_count, Fat_struct.disc_handle);

    //Zwraca liczbę odczytanych elementów
	return count;
}

//Funkcja ładuje do uchwytu dysk za pomocą funckji fopen
int load_disc(const char *file)
{
    //Jesli przekazana ścieżka jest błędna funkcja zwraca 1
    if(file == NULL)
    {
        return 1;
    }

    //Otworzenie pliku za pomocą fopen do uchwytu
    Fat_struct.disc_handle = fopen(file, "rb");

    //Jeśli coś poszło nie tak funkcja zwraca 1
    if(Fat_struct.disc_handle == NULL)
    {
        return 1;
    }
}

//Funkcja ładuje dane do struktury i odczytuje boot_sector
int load_fat()
{   
    //Zapisuje pierwszą ścieżke do tablicy
    Fat_struct.current_folder[0] = '/';

    //Wczytuje boot_sector, z pozycji 0, wczytując jeden SECTOR_SIZE
    int check = readblock(&Fat_struct.boot_sector, 0, 1);

    //Jeśli coś poszło nie tak to funkcja zwraca 1
    if(check == 0)
    {
        return 1;
    }

    //Zapisuje dodatkowe dane, które będą przydatne
    Fat_struct.fat_cluster_size = Fat_struct.boot_sector.parameters.sectors_per_cluster * Fat_struct.boot_sector.parameters.bytes_per_sector;
    Fat_struct.adress_of_main_dir = (Fat_struct.boot_sector.parameters.reserved_sectors + (Fat_struct.boot_sector.parameters.sectors_per_fat * Fat_struct.boot_sector.parameters.fat_count)) * Fat_struct.boot_sector.parameters.bytes_per_sector;
    Fat_struct.adrees_of_cluster_area = Fat_struct.adress_of_main_dir + Fat_struct.boot_sector.parameters.entries_in_root_directory * 32;
    Fat_struct.fat_entries_count = Fat_struct.boot_sector.parameters.sectors_per_fat * Fat_struct.boot_sector.parameters.bytes_per_sector / 2;
    Fat_struct.fat_size = Fat_struct.fat_entries_count*2;

    //Utworzenie dwóch tablic pomocniczych do sprawdzenia poprawności
    uint8_t fat[Fat_struct.fat_size];
    uint8_t fat_copy[Fat_struct.fat_size];

    //Obliczenie danych które pomogą wyliczyć na którym sektorze znajduje się FAT
    uint32_t fat_start = (Fat_struct.boot_sector.parameters.reserved_sectors + Fat_struct.boot_sector.parameters.sectors_per_fat) * Fat_struct.boot_sector.parameters.bytes_per_sector;;
    uint32_t sector_number = fat_start / SECTOR_SIZE;
    uint32_t sector_offset = fat_start % SECTOR_SIZE;
    uint32_t size_disc_sector = ((Fat_struct.fat_size+size_disc_sector)/SECTOR_SIZE);

    //Stworzenie tablicy do zapisu danych
    uint8_t buffer[size_disc_sector*SECTOR_SIZE];

    //Wczytanie odpowiednio wyliczonego sektora z FAT1
    check = readblock(buffer, sector_number, size_disc_sector);

    //Jeśli coś poszło nie tak do funkcja zwraca 1
    if(check != size_disc_sector)
    {
        return 1;
    }

    //Kopiuje wszystkie pobrane elemnty do osobnej tablicy 
    memcpy(fat, buffer+sector_offset, Fat_struct.fat_size);

    //Ponowne wyliczenie gdzie znajduje się tablica FAT2
    fat_start = (Fat_struct.boot_sector.parameters.reserved_sectors + Fat_struct.boot_sector.parameters.sectors_per_fat) * Fat_struct.boot_sector.parameters.bytes_per_sector;
    sector_number = fat_start / SECTOR_SIZE;
    sector_offset = fat_start % SECTOR_SIZE;
    size_disc_sector = ((Fat_struct.fat_size+size_disc_sector)/SECTOR_SIZE) + 1;

    //Stworzenie tablicy do zapisu danych
    uint8_t buffer2[size_disc_sector*SECTOR_SIZE];

    //Wczytanie odpowiednio wyliczonego sektora z FAT1
    check = readblock(buffer2, sector_number, size_disc_sector);

    //Jeśli coś poszło nie tak do funkcja zwraca 1
    if(check != size_disc_sector)
    {
        return 1;
    }

    //Kopiuje wszystkie pobrane elemnty do osobnej tablicy 
    memcpy(fat_copy, buffer2+sector_offset, Fat_struct.fat_size);

    //Osobna funkcja sprawdza czy dwie porabne tablice są identyczne
    //Jesli nie to funkcja zwraca błąd (1)
    check = fat_check(fat, fat_copy);

    //Jeśli tablice nie są takie same funkcja zwraca 1
    if(check == 1)
    {
        printf("Blad porownywania zawartosci tablic FAT\n");
        return 1;
    }

    //Zaalokowanie osobnej tablicy na FAT1, którą można wykorzystac później
    Fat_struct.fat_table = (uint32_t*)malloc(Fat_struct.fat_entries_count*4);

    //Jeśli alokacja się nie powiodła funkcja zwraca 1
    if(Fat_struct.fat_table == NULL)
    {
        return 1;
    }

    //Przepisuje dane z pomocniczej tablicy do głównej
    for(int i = 0; i<Fat_struct.fat_entries_count; i++)
    {
        Fat_struct.fat_table[i] = ((uint16_t*)fat)[i];
    }

    //Jeśli wszystko się udało funkcja zwraca 0
    return 0;
}

//Funkcja sprawdza czy dwie tablice są poprawne
int fat_check(uint8_t fat[], uint8_t fat_copy[])
{
    for(uint32_t i = 0; i < Fat_struct.fat_size; i++)
    {
        //Jeśli tablice się różnią funkcja zwraca 1
        if(fat[i] != fat_copy[i])
        {
            return 1;
        }
    }

    //Jeśli wszystko się udało funkcja zwraca 0
    return 0;
}

//Funkcja szuka określone wpisu który zaczyna się w postion i ma nazwe taka sama jak ta podana w argumencie
int load_entry_name(uint32_t position, const char *name, struct DIR_ENTRY_DATA *result)
{
    //Jeśli argumenty podane są błędne to funkcja zwraca 1
    if(name == NULL || result == NULL)
    {
        return 1;
    }

    //Obliczenie liczby wpisów
    uint32_t entries_count = 0;
    struct DIR_ENTRY_DATA temp;

    //Jeśli position jest równe 0 to znajdujemy się w katalogu głównym
    //Liczba wpisów jest zapisana w strukturze pomocniczej
	if(position == 0)
    {
        entries_count = Fat_struct.boot_sector.parameters.entries_in_root_directory;
    }
    //Jeśli position jest inny niż 0 to znajdujemy sie w podkatalogu
    else
    {  
        //W pętli przechodzimy kolejne wpisy
        for(uint32_t i = 0; ; i++)
        {
            //Za pomocą funkcji wczytuje wpis, z danego klastra i określonej pozycji
            int check = load_entry_pos(position, i, &temp);

            //Jeśli coś poszło nie tak liczba wpisów jest równa 0 o przerywa pętle
            if(check == 1) 
            {
                entries_count = 0;
                break;
            }

            //Jeśli wpis jest ostatni przerywamy petle
            if(temp.name[0] == 0x00) 
            {
                break;
            }

            //Dodajemy +1 do liczby wpisów
            entries_count++;
        }
    }

    //Przeszukujemy wszystkie wpisy ze wcześniej zliczonej ich liczby
    for(uint32_t i = 0; i < entries_count; i++)
    {
        //Za pomocą funkcji wczytuje wpis, z danego klastra i określonej pozycji
        int check = load_entry_pos(position, i, &temp);

        //Określamy za jaki plik odpowiedzialny jest wpis
        bool check_entry;

        if(temp.name[0] == 0x00 || temp.name[0] == 0x05 || temp.name[0] == 0xE5)
        {
            check_entry = false;
        }
        else if(temp.attribute & 0x04 || temp.attribute & 0x08)
        {
            check_entry = false;
        }
        else
        {
            check_entry = true;
        }

        //Jeśli wczytanie wpisu lub plik jest związany z nie odpowiednim plikiem
        //Pętla rozpoczyna kolejny obieg
        if(check || check_entry == false)
        {
            continue;
        }

        //Tablica do której zapiszemy nowo stworzoną nazwę
        char combname[257];

        //Za pomocą funckji łączymy nazwe z rozszerzeniem tworząc nową nazwe
        combining_the_name((char*)temp.name, (char*)temp.extension, combname);

        //Porównujemy stworzoną nazwę z ta podaną jako argument
        if(strcmp(name, combname) == 0)
        {
            //Jesli nazwy są takie same to wpis jest zapisywany do argumentu zwrotnego
            //Funkcja zwraca 0
            *result = temp;
            return 0;
        }
    }

    //Funckja jeśli nie powiodlo się szukanie wpisu funkcja zwraca 1
    return 1;
}

//Funkcja szuka wpisu z pod podanej scieżki podanej w argumencie
int load_entry_path(const char *path, struct DIR_ENTRY_DATA *result)
{
    //Jeśli argumenty podane są błędne to funkcja zwraca 1
    if(path == NULL || result == NULL)
    {
        return 1;
    }

    //Kopiujemy scieżke do osobnej danej
    char *temp = strdup(path);

    //Ucinamy scięzke przy pierwszym znaku "/"
    char *path_cut = strtok(temp, "/");

    //jeśli coś poszło nie tak funkcja zwraca 1
    if(path_cut == NULL) 
    {
        return 1;
    }

    //Rozpoczęcie od katalogu głownego
    uint32_t current_dir = 0;
    bool check_directory = true;
    struct DIR_ENTRY_DATA entry;

    //Przechodzenie po tokenach i sprawdzanie kolejnych wpisów
    while(path_cut != NULL)
    {
        //Sprawdzenie czy w scięzce nie ma pliku
        //Jeśli jest to funkcja zwraca 1
        if(check_directory == false) 
        { 
            free(temp);
            return 1; 
        }

        //Wczytanie kolejnego wpisu po znanej nazwie
        int check = load_entry_name(current_dir, path_cut, &entry);

        //Jesli coś poszlo nie tak funkcja zwraca 1
        if(check != 0) 
        {   
            free(temp);
            return 1; 
        }

        //Określenie kolejnego katalogu
        current_dir = entry.file_start;

        //Sprawdzenie czy to katolog czy plik
        check_directory = entry.attribute & 0x10;

        //Uciecie kolejnej nazwy przy "/"
        path_cut = strtok(NULL, "/");
    }

    //Po przejściu pętli zapisuje wpis do argumentu wyjściowego
    *result = entry;

    //Zwolnienie pamięci tokenów i zwrócenie 0
    free(temp);
    return 0;
}

//Funkcja szuka wpisu który rozpoczyna się w określonej klastrze na podanej pozycji
int load_entry_pos(uint32_t position_start, uint32_t position, struct DIR_ENTRY_DATA *result)
{
    //Jeśli argumenty podane są błędne to funkcja zwraca 1
    if(result == NULL)
    {
        return 1;
    }

    //Przeszukanie katalogu głównego
    if(position_start == 0)
    {
        //Obliczenie sektora gdzie ma znajdować się wpis
        uint32_t entry_address = Fat_struct.adress_of_main_dir + position * 32;
        uint32_t sector_number = entry_address / SECTOR_SIZE;
        uint32_t sector_offset = entry_address % SECTOR_SIZE;

        //Tablica do zapisu danych
        uint8_t buffer[SECTOR_SIZE];

        //Wczytywanie do buffora, z pod danej pozycji jednego elementu
        int check = readblock(buffer, sector_number, 1);

        //Jeśli coś poszło nie tak funkcja zwraca 1
        if(check != 1)
        {
            return 1;
        } 

        struct DIR_ENTRY_DATA *temp = (struct DIR_ENTRY_DATA *) (buffer+ sector_offset);

        //Zapis wpisu do argumentu wyjściowego i zwrócenie 0
        *result = *temp;
        return 0;
    }
    //Jeśli pozycja jest inna niz 0 to szukamy w podkatalogu
    else
    {
        //Obliczenie przesunięcia
        uint32_t entry_offset = position * 32;

        //Określenie klastra gdzie ma znajdowac się wpis
        uint32_t current_cluster = position_start;

        //Przesunięcie offsetu oraz zapis klastra
        while (entry_offset >= Fat_struct.fat_cluster_size) 
        {
            current_cluster = Fat_struct.fat_table[current_cluster];
            entry_offset = entry_offset - Fat_struct.fat_cluster_size;
        }

        //Tablicy do zapisu danych
        uint8_t buffer[Fat_struct.fat_cluster_size];

        //Określienie dokładnego adresu od którego mamy zacząc czytanie danych
        uint32_t address = (Fat_struct.adrees_of_cluster_area + (current_cluster - 2) * Fat_struct.fat_cluster_size)/SECTOR_SIZE;

        //Wczytanie danych do buffora, z pod obliczone adresu
        int check = readblock(buffer, address, Fat_struct.fat_cluster_size/SECTOR_SIZE);

        //Jesli liczba wczytanych danych jest inna niż podana funkcja zwraca 1
        if(check != Fat_struct.fat_cluster_size/SECTOR_SIZE)
        {
            return 1;
        }

        struct DIR_ENTRY_DATA *temp = (struct DIR_ENTRY_DATA *) (buffer + entry_offset);

        //Zapis wpisu do argumentu wyjściowego
        *result = *temp;
        return 0;
    }
}

//Funkcja wczytuje dane o pliku do struktury
int load_filedata(const char *path, struct FILE_DATA *file_data)
{
    //Jeśli argumenty podane są błędne to funkcja zwraca 1
	if(path == NULL || file_data == NULL) 
    {
        return 1;
    }

    //Wczytanie wpisu odpowiedniego wpisu
    struct DIR_ENTRY_DATA temp;

    //Odczytanie wpisu z podanej ścieżki
	int check = load_entry_path(path, &temp);

    //Jeśli coś poszło nie tak funkcja zwraca 1
	if(check != 0)
    {
        return 1;
    }

    //Odczytanie atrybutów pliku z wpisu
    file_data->read_only = temp.attribute & 0x01;
	file_data->hidden = temp.attribute & 0x02;
	file_data->system = temp.attribute & 0x04;
	file_data->volume = temp.attribute & 0x08;
    file_data->directory = temp.attribute & 0x10;
	file_data->archive = temp.attribute & 0x20;
    
    //Odczytanie czasu stworzenia
    struct TIME cre;
    cre.min = (temp.creation_time & 0x07E0) >> 5;
	cre.hour = (temp.creation_time & 0xF800) >> 11;
	cre.day = (temp.creation_date & 0x001F);
	cre.month = ((temp.creation_date & 0x01E0) >> 5) - 1;
	cre.year = ((temp.creation_date & 0xFE00) >> 9) + 80;

    //Odczytanie czasu modyfikacji
    struct TIME mod;
    mod.min = (temp.modification_time & 0x07E0) >> 5;
	mod.hour = (temp.modification_time & 0xF800) >> 11;
	mod.day = (temp.modification_date & 0x001F);
	mod.month = ((temp.modification_date & 0x01E0) >> 5) - 1;
	mod.year = ((temp.modification_date & 0xFE00) >> 9) + 80;

    //Odczytanie czasu dostepu
    struct TIME acc;
    acc.min = (0 & 0x07E0) >> 5;
	acc.hour = (0 & 0xF800) >> 11;
	acc.day = (temp.access_date & 0x001F);
	acc.month = ((temp.access_date & 0x01E0) >> 5) - 1;
	acc.year = ((temp.access_date & 0xFE00) >> 9) + 80;

    //Dostosowanie odpowiednio daty
	file_data->create_time = cre;
    if(file_data->create_time.hour + 2 >= 24)
    {
        file_data->create_time.hour -= 24;
        file_data->create_time.day += 1;
    }

	file_data->modify_time = mod;
    if(file_data->modify_time.hour + 2 >= 24)
    {
        file_data->modify_time.hour -= 24;
        file_data->modify_time.day += 1;
    }

	file_data->access_time = acc;
    if(file_data->access_time.hour + 2 >= 24)
    {
        file_data->access_time.hour -= 24;
        file_data->access_time.day += 1;
    }

    //Odczyt rozmiaru pliku
	file_data->size = temp.file_size;
	
    //Sprawdzenie dlugości łancucha klastró
    uint32_t count = 0;

    //Aktualny numer klastra
    uint32_t current_cluster = temp.file_start;

    //Przechodzenie w pętli do kolejnych klastrów
    for(uint32_t i = 0; i < Fat_struct.fat_entries_count; i++)
    {   
        //Jeśli klaster jest kończący albo 0 przerywa pętle
        if(current_cluster >= 0xFFF8 || current_cluster == 0)
        {
            break;
        }
        
        //Dodaje długosć do łańcucha i przechodzi do kolejengo klastra
        count++;
        current_cluster = Fat_struct.fat_table[current_cluster];
    }

    //Zapisuje długość łąncucha klastrów
	file_data->clusters_count = count;

    //Zapisuje klaster początkowy
    file_data->first_cluster = temp.file_start;
	
	return 0;
}

struct DIR_HANDLE *opendir(const char *path)
{
    //Wczytanie danych do struktury
	struct FILE_DATA file_info;
	int check = load_filedata(path, &file_info);

    //Sprawdzenie czy nie jestęsmy na początku scieżki
	if(strcmp(path, "/") != 0)
    {
        //Sprawdzamy czy napewno wczytaliśmy folder
        if(file_info.directory == 0)
        {
            return NULL;
        }
    } 

    //Określenie pierwszego klastra
	uint16_t first_cluster;

    //Jeśli wczytywanie danych zwróciło 1 pierwszy klaster to 0
    if(check == 1)
    {
        first_cluster = 0;
    }
    //Jeśli nie to pierwszy klaster wczytujye ze struktury
    else
    {
        first_cluster = file_info.first_cluster;
    }
    
    //Tworzymy uchwyt do folderu
	struct DIR_HANDLE *dir_handle = (struct DIR_HANDLE*)malloc(sizeof(struct DIR_HANDLE));

    //jeśli alokacja przebiegla błędnie zwracamy NULL
	if(dir_handle == NULL)
    {
        return NULL;
    }

    //Zapisuje pierwszy klaster do uchwytu oraz pozycje 
	dir_handle->start = first_cluster;
	dir_handle->pos = 0;

    //Zwracamy uchwyt do folderu
	return dir_handle;
}

void closedir(struct DIR_HANDLE *dir_handle)
{
    //Zwalniamy alokacje czyli zamykamy folder
	free(dir_handle);
}

int readdir(struct DIR_HANDLE *dir_handle, struct DIR_ENTRY *entry)
{
    //Jeśli argumenty podane są błędne to funkcja zwraca 1
    if(dir_handle == NULL || entry == NULL) 
    {
        return 1;
    }

    //odczytywnie kolejnych wpisów
    while(1)
    {
        struct DIR_ENTRY_DATA temp;

        //Załadowanie wpis podając pozycje
        int check = load_entry_pos(dir_handle->start, dir_handle->pos, &temp);

        //Jesli coś poszło nie tak funkcja zwraca 1
        if(check == 1) 
        {
            return 1;
        }

        //Jeśli nazwy nie ma funkcja zwraca 1
        if(temp.name[0] == 0)
        {
            return 1;
        }

        //SPrawdzenie czy wpis jest związany z odpowiednim plikiem
        bool check_entry;
        if(temp.name[0] == 0x00 || temp.name[0] == 0x05 || temp.name[0] == 0xE5)
        {
            check_entry = false;
        }
        else if(temp.attribute & 0x04 || temp.attribute & 0x08)
        {
            check_entry = false;
        }
        else
        {
            check_entry = true;
        }

        //Sprawdzenie czy wpis jest dobry
        if(check_entry == true)
        {
            //Stworzenie nawzwy z wyciągnietej nazwy oraz rozszerzenia
            combining_the_name(temp.name, temp.extension, entry->filename);

            //przesunięcie pozycji w uchwycie
            dir_handle->pos++;

            //Zwrócenie 0
            return 0;
        }

        //przesunięcie pozycji w uchwycie
        dir_handle->pos++;
    }
}

struct FILE_HANDLE *openfile(const char *path, const char* mode)
{
	struct FILE_DATA file_info;

    //Załadownie do struktury informacji o pliku
	int check = load_filedata(path, &file_info);

    //Jeśli coś poszło nie tak zwraca NULL
	if(check == 1) 
    {
        return NULL;
    }

    //Sprawdza czy mamy do czynienia z plikiem
    //jeśli nie zwraca NULL
    if(file_info.directory == 1)
    {
        return NULL;
    }

    //Stworzenie uchwytu do pliku
	struct FILE_HANDLE *file_handle = (struct FILE_HANDLE *)malloc(sizeof(struct FILE_HANDLE));

    //Jesli alokacja sie nie powiodła funkcja zwraca NULL
	if(file_handle == NULL) 
    {
        return NULL;
    }

    //Przekopiowanie ścieżki do uchwytu
	file_handle->path = strdup(path);

    //Jeśli coś poszło nie tak funkcja zwraca NULL
	if(file_handle->path == NULL) 
    {
        return NULL;
    }  

    //Zapisuje pozycje w pliku na 0
	file_handle->pos = 0;

    //Zwraca uchwyt do pliku
	return file_handle;
}

void closefile(struct FILE_HANDLE *file_handle)
{
    //Zwalnianie alokacji, zamykanie pliku
    free(file_handle->path);
}

int readfile(void *buffer, uint32_t size, struct FILE_HANDLE *file_handle)
{
    //Jeśli argumenty podane są błędne to funkcja zwraca 1
    if(buffer==NULL || file_handle==NULL)
    {
        return 0;
    }

    struct DIR_ENTRY_DATA temp;

    //Załadowanie wpisu po podanej ścieżce z uchwytu
    int check = load_entry_path(file_handle->path, &temp);

    //Jeśli coś poszlo nie tak funkcja zwraca 0
    if(check == 1)
    {
        return 0;
    } 

    //Jeśli pozycja w pliku jest taka sama jak rozmiar pliku,
    //Doszlismy do końca pliku funkcja zwraca -1
    if(file_handle->pos >=temp.file_size)
    {
        return -1;
    }

    //Zapisanie klastra oraz pozycji w pliku
	uint32_t current_cluster = temp.file_start;
    uint32_t cluster_offset = file_handle->pos;

    //Przesunięcie offsetu oraz przejście do kolejnego klastra
	while(cluster_offset >= Fat_struct.fat_cluster_size)
	{
        current_cluster = Fat_struct.fat_table[current_cluster];
		cluster_offset -= Fat_struct.fat_cluster_size;
	}

	uint32_t bytes_number = 0;
	uint8_t cluster[Fat_struct.fat_cluster_size];

    //Określeniu adresu z pod ktorego nalezy wczytać dane
    uint32_t address = Fat_struct.adrees_of_cluster_area + (current_cluster - 2) * Fat_struct.fat_cluster_size;

    //Wczytanie danych do buffora z pod określonego adresu
    check = readblock(cluster, address/SECTOR_SIZE, Fat_struct.fat_cluster_size/SECTOR_SIZE);

    for(uint32_t i = 0; i < size; i++)
    {
        //Zapisywanie danych do buffora podanego w argumencie
        ((uint8_t *)buffer)[i] = cluster[cluster_offset];

        //Dodawanie do offsetu oraz liczby przeczytanych bajtów
        cluster_offset++;
        bytes_number++;

        //Jeśli offset jest dalej niż rozmiar klastra wczytuje klejny klaster
        if(cluster_offset >= Fat_struct.fat_cluster_size)
        {  
            //Ustawienie offsetu na 0
            cluster_offset = 0;

            //Przejście do kolejnego klastra
            current_cluster = Fat_struct.fat_table[current_cluster];

            //Określeniu adresu z pod ktorego nalezy wczytać dane
            uint32_t address = Fat_struct.adrees_of_cluster_area + (current_cluster - 2) * Fat_struct.fat_cluster_size;

            //Wczytanie danych do buffora z pod określonego adresu
            int check = readblock(cluster, address/SECTOR_SIZE, Fat_struct.fat_cluster_size/SECTOR_SIZE);

            //Jeśli wczytano za mało danych przerywa pętle
            if(check != Fat_struct.fat_cluster_size/SECTOR_SIZE) 
            {
                break;
            }
        }
    }

    //ustawienie nowej pozycji w uchwycie
    file_handle->pos += bytes_number;

    //Zwrócenie liczby przeczytanych bajtów
	return bytes_number;
}

void dir_function()
{
    struct DIR_HANDLE *main_dir = opendir(Fat_struct.current_folder);
    if(main_dir == NULL)
    {
        printf("Nie mozna otworzyc folderu\n");
        return;
    }

    struct DIR_ENTRY data;
    int check = readdir(main_dir, &data);

    while(check == 0)
    {
        char name[200];
        if(strcmp(Fat_struct.current_folder, "/") != 0)
        {
            strcpy(name, Fat_struct.current_folder);
            strcat(name, "/");
            strcat(name, data.filename);
        }
        else
        {
            strcpy(name, Fat_struct.current_folder);
            strcat(name, data.filename);
        }

        struct FILE_DATA file_info;
        int check_load = load_filedata(name, &file_info);
        if(check_load == 0)
        {
            printf("%02d/%02d/%04d  ", file_info.modify_time.day, file_info.modify_time.month + 1, file_info.modify_time.year + 1900);
            printf("%02d:%02d     ", file_info.modify_time.hour+2, file_info.modify_time.min);
            if(file_info.directory == 1)
            {
                printf("<FOLDER> ");
            }
            else
            {
                printf("%8d ", file_info.size);
            }
            printf("%s\n", data.filename);
        }
        check = readdir(main_dir, &data);
    } 
    closedir(main_dir);
}

void cd_function(char *folder)
{
    char temp[200];
    if(strcmp(Fat_struct.current_folder, "/") != 0)
    {
        strcpy(temp, Fat_struct.current_folder);
        strcat(temp, "/");
        strcat(temp, folder);
    }
    else
    {
        strcpy(temp, Fat_struct.current_folder);
        strcat(temp, folder);
    }

    struct DIR_HANDLE *check_dir = opendir(temp);
    if(check_dir == NULL)
    {
        printf("Nie mozna otworzyc folderu\n");
        return;
    }

    if(strcmp(folder, ".") == 0)
    {
        return;
    }
    if(strcmp(folder, "..") == 0)
    {
        char *back = strrchr(Fat_struct.current_folder, '/');
        if(back == NULL)
        {
            return;
        }
        if(back == Fat_struct.current_folder)
        {
            strcpy(Fat_struct.current_folder, "/");
        }
        else
        {
            *back = 0;
        }
    }
    else
    {
        strcpy(Fat_struct.current_folder, temp);
    }
    
    closedir(check_dir);
}

void pwd_function()
{
    printf("%s\n", Fat_struct.current_folder);
}

void fileinfo_function(char *filename)
{
    char name[200];
    if(strcmp(Fat_struct.current_folder, "/") != 0)
    {
        strcpy(name, Fat_struct.current_folder);
        strcat(name, "/");
        strcat(name, filename);
    }
    else
    {
        strcpy(name, Fat_struct.current_folder);
        strcat(name, filename);
    }

    struct FILE_DATA file_info;
    int check = load_filedata(name, &file_info);
    if(check == 1)
    {
        printf("Blad nazwy pliku\n");
        return;
    }

    printf("Pelna nazwa: %s\n", name);
    printf("Atrybuty: \n");
    if(file_info.archive == 1)
    {
        printf("    A+\n");
    }
    else
    {
        printf("    A-\n");
    }

    if(file_info.read_only == 1)
    {
        printf("    R+\n");
    }
    else
    {
        printf("    R-\n");
    }

    if(file_info.system == 1)
    {
        printf("    S+\n");
    }
    else
    {
        printf("    S-\n");
    }

    if(file_info.hidden == 1)
    {
        printf("    H+\n");
    }
    else
    {
        printf("    H-\n");
    }

    if(file_info.directory == 1)
    {
        printf("    D+\n");
    }
    else
    {
        printf("    D-\n");
    }

    if(file_info.volume == 1)
    {
        printf("    V+\n");
    }
    else
    {
        printf("    V-\n");
    }
    printf("Rozmiar pliku: %u", file_info.size);
    printf(" bajty\n");

    printf("Ostatni zapis: ");
    printf("%02d/%02d/%04d  ", file_info.modify_time.day, file_info.modify_time.month + 1, file_info.modify_time.year + 1900);
    printf("%02d:%02d\n", file_info.modify_time.hour+2, file_info.modify_time.min);
    printf("Ostatni dostep: ");
    printf("%02d/%02d/%04d  ", file_info.access_time.day, file_info.access_time.month + 1, file_info.access_time.year + 1900);
    printf("Utworzono: ");
    printf("%02d/%02d/%04d  ", file_info.create_time.day, file_info.create_time.month + 1, file_info.create_time.year + 1900);
    printf("%02d:%02d\n", file_info.create_time.hour+2, file_info.create_time.min);

    printf("Lancuch klastrow: ");
    uint32_t current_cluster = file_info.first_cluster;

    if(file_info.clusters_count == 0)
    {
        printf("0\n");
    }
    else if(file_info.clusters_count > 0)
    {
        printf("[%hu] ", current_cluster);
    }

    for(uint32_t i = 1; i < file_info.clusters_count; i++)
    {
        current_cluster = Fat_struct.fat_table[current_cluster];
        printf("%u ", current_cluster);
    }

    printf("\nLiczba klastrow: %u\n", file_info.clusters_count);
}

void cat_function(char *filename)
{
    char name[200];
    if(strcmp(Fat_struct.current_folder, "/") != 0)
    {
        strcpy(name, Fat_struct.current_folder);
        strcat(name, "/");
        strcat(name, filename);
    }
    else
    {
        strcpy(name, Fat_struct.current_folder);
        strcat(name, filename);
    }

    struct FILE_DATA file_info;
    int check = load_filedata(name, &file_info);
    if(check == 1)
    {
        printf("Blad nazwy pliku\n");
        return;
    }

    if(file_info.directory == 1)
    {
        printf("Podano folder a nie plik\n");
        return;
    }

    struct FILE_HANDLE *file = openfile(name, "r");
    if(file == NULL)
    {
        printf("Blad otwierania pliku\n");
        return;
    }

    char data[101];
    while(1)
    {
        int check = readfile(data, 100, file);
        if(check == -1)
        {
            break;
        }
        else
        {
            data[check] = 0;
            printf("%s",data);
        }
    }
    printf("\n");
    closefile(file);
}

void spaceinfo_function()
{
    uint32_t damaged_cluster = 0;
    uint32_t busy_cluster = 0;
    uint32_t free_cluster = 0;
    uint32_t end_cluster = 0;

    for(uint32_t i = 0; i < Fat_struct.fat_entries_count; i++)
    {
        if(Fat_struct.fat_table[i] == 0x0000)
        {
            free_cluster++;
        }
        else if(Fat_struct.fat_table[i] >= 0xFFF8)
        {
            end_cluster++;
        }
        else if(Fat_struct.fat_table[i] == 0xFFF7)
        {
            damaged_cluster++;
        }
        else
        {
            busy_cluster++;
        }
    }

    printf("Zajete klastry: %u\n", busy_cluster);
    printf("Wolne klastry: %u\n", free_cluster);
    printf("Uszkodzone klastry: %u\n", damaged_cluster);
    printf("Konczace klastry: %u\n", end_cluster);
    
    printf("Klastry w bajtach: %u\n", Fat_struct.boot_sector.parameters.sectors_per_cluster * Fat_struct.boot_sector.parameters.bytes_per_sector);
    printf("Klastry w sektorach: %u\n", Fat_struct.boot_sector.parameters.sectors_per_cluster);
}

void rootinfo_function()
{
    struct DIR_ENTRY_DATA temp;
    int entry;
    for(int i = 0; i < Fat_struct.boot_sector.parameters.entries_in_root_directory; i++)
    {
        int check = load_entry_pos(0, i, &temp);
        if(check == 1)
        {
            return;
        }

        if(temp.name[0] == 0x00)
        {
            entry++;
        }
        else if(temp.name[0] == 0x05)
        {
            entry++;
        }
        else if(temp.name[0] == 0xE5)
        {
            entry++;
        }
    }

    printf("Wpisy w katalogu glownym: %u\n", Fat_struct.boot_sector.parameters.entries_in_root_directory - entry);
    printf("Maksymalna liczba wpisow: %u\n", Fat_struct.boot_sector.parameters.entries_in_root_directory);
    printf("Procentowe wypelnienie: %.2f%%\n", 100.0*(Fat_struct.boot_sector.parameters.entries_in_root_directory - entry)/(Fat_struct.boot_sector.parameters.entries_in_root_directory));
}

void get_function(char *filename)
{
    char name[200];
    if(strcmp(Fat_struct.current_folder, "/") != 0)
    {
        strcpy(name, Fat_struct.current_folder);
        strcat(name, "/");
        strcat(name, filename);
    }
    else
    {
        strcpy(name, Fat_struct.current_folder);
        strcat(name, filename);
    }

    struct FILE_DATA file_info;
    int check = load_filedata(name, &file_info);
    if(check == 1)
    {
        printf("Blad nazwy pliku\n");
        return;
    }

    if(file_info.directory == 1)
    {
        printf("Podano folder a nie plik\n");
        return;
    }

    struct FILE_HANDLE *file = openfile(name, "r");
    if(file == NULL)
    {
        printf("Blad otwierania pliku\n");
        return;
    }

    FILE *file_output = fopen(filename, "w");
    if(file_output == NULL)
    {
        printf("Nie mozna stworzyc pliku\n");
        closefile(file);
        return;
    }

    char data[100];
    while(1)
    {
        int number_read = readfile(data, 100, file);
        if(number_read == -1)
        {
            break;
        }
        else
        {
            fwrite(data, 1, number_read, file_output);
        }
    }
    printf("Utworzono plik w katalogu z programem\n");
    closefile(file);
    fclose(file_output);
}

void zip_function(char *filename1, char *filename2, char *filename3)
{
    char name1[200];
    if(strcmp(Fat_struct.current_folder, "/") != 0)
    {
        strcpy(name1, Fat_struct.current_folder);
        strcat(name1, "/");
        strcat(name1, filename1);
    }
    else
    {
        strcpy(name1, Fat_struct.current_folder);
        strcat(name1, filename1);
    }

    char name2[200];
    if(strcmp(Fat_struct.current_folder, "/") != 0)
    {
        strcpy(name2, Fat_struct.current_folder);
        strcat(name2, "/");
        strcat(name2, filename2);
    }
    else
    {
        strcpy(name2, Fat_struct.current_folder);
        strcat(name2, filename2);
    }

    struct FILE_DATA file_info1;
    int check = load_filedata(name1, &file_info1);
    if(check == 1)
    {
        printf("Blad nazwy pliku\n");
        return;
    }

    if(file_info1.directory == 1)
    {
        printf("Podano folder a nie plik\n");
        return;
    }

    struct FILE_DATA file_info2;
    check = load_filedata(name2, &file_info2);
    if(check == 1)
    {
        printf("Blad nazwy pliku\n");
        return;
    }

    if(file_info2.directory == 1)
    {
        printf("Podano folder a nie plik\n");
        return;
    }

    struct FILE_HANDLE *file1 = openfile(name1, "r");
    if(file1 == NULL)
    {
        printf("Blad otwierania pliku\n");
        return;
    }

    struct FILE_HANDLE *file2 = openfile(name2, "r");
    if(file2 == NULL)
    {
        printf("Blad otwierania pliku\n");
        return;
    }

    FILE *file_output = fopen(filename3, "wb");
    if(file_output == NULL)
    {
        printf("Nie mozna stworzyc pliku\n");
        closefile(file1);
        closefile(file2);
        return;
    }

    int check_end1 = 0;
    int check_end2 = 0;
    char c = 0;
    while(1)
    {
        while(c != '\n' && check_end1 != -1)
        {
            check_end1 = readfile(&c, 1, file1);
            if(check_end1 != -1)
            {
                fwrite(&c, 1, 1, file_output);
            }
        }
        
        c = 0;
        while(c != '\n' && check_end2 != -1)
        {
            check_end2 = readfile(&c, 1, file2);
            if(check_end2 != -1)
            {
                fwrite(&c, 1, 1, file_output);
            }
        }
        
        c = 0;
        if(check_end1 == -1 && check_end2 == -1)
        {
            break;
        }
    }

    printf("Utworzono plik w katalogu z programem\n");
    closefile(file1);
    closefile(file2);
    fclose(file_output);
}

char *combining_the_name(const char *filename, const char *fileextension, char *result)
{
	char name[9];
	memcpy(name, filename, 8);
	name[8] = 0;
	for(int i=7; name[i]==' ' && i>=0; i--)
    {
        name[i] = 0;
    }

	char ext[4];
	memcpy(ext, fileextension, 3);
	ext[3] = 0;
	for(int i=2; ext[i]==' ' && i>=0; i--)
    {
        ext[i] = 0;
    }

	*result = 0;
	strcat(result, name);

	if(*ext != 0)
	{
		strcat(result, ".");
		strcat(result, ext);
	}

	return result;
}
