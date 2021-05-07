#include <iostream>
#include <SDL.h>
#include <cstring>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>

#define WIDTH 800
#define HEIGHT 600

class File {
    public:
        std::string name;
        bool is_dir;
};


void initialize(SDL_Renderer *renderer);
void render(SDL_Renderer *renderer);

std::vector<File*> getItemsInDirectory(std::string dirpath);
void freeItemVector(std::vector<File*> *vector_ptr);

int main(int argc, char **argv)
{
    char *home = getenv("HOME");
    printf("HOME: %s\n", home);

    // initializing SDL as Video
    SDL_Init(SDL_INIT_VIDEO);

    // create window and renderer
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer);

    // ! EXAMPLE --------------------------------------------------------
    // log home dir contents to console
    std::vector<File*> file_v = getItemsInDirectory(home);
    for(int i = 0; i < file_v.size(); i++) {
        if(file_v[i]->is_dir) {
            printf("\x1b[34m%s/\x1b[0m\n",file_v[i]->name.c_str());
        } else {
            printf("%s\n",file_v[i]->name.c_str());
        }
    }
    freeItemVector(&file_v);
    // ------------------------------------------------------------------

    // initialize and perform rendering loop
    initialize(renderer);
    render(renderer);
    SDL_Event event;
    SDL_WaitEvent(&event);
    while (event.type != SDL_QUIT)
    {
        //render(renderer);
        SDL_WaitEvent(&event);
    }

    // clean up
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void initialize(SDL_Renderer *renderer)
{
    // set color of background when erasing frame
    SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255);
}

void render(SDL_Renderer *renderer)
{
    // erase renderer content
    SDL_RenderClear(renderer);
    
    // TODO: draw!

    // show rendered frame
    SDL_RenderPresent(renderer);
}

/** Get all the file/directory items at the given dirpath
 * @param dirpath Path of the directory to get the contents of.
 * @return A vector of files and folders inside the directory.
 */
std::vector<File*> getItemsInDirectory(std::string dirpath)
{
    std::vector<File*> file_vector;
    struct stat info;
    int err = stat(dirpath.c_str(), &info);
    if(err == 0 && S_ISDIR(info.st_mode))
    {
        DIR* dir = opendir(dirpath.c_str());
        struct dirent *entry;
        
        while((entry = readdir(dir)) != NULL)
        {
            File* file_entry = new File();
            file_entry->name = entry->d_name;
            file_entry->is_dir = (entry->d_type == DT_DIR);
            file_vector.push_back(file_entry);
        }
    }
    return file_vector;
}

/** Frees the memory of the items in the item vector
 * @param vector_ptr a pointer to the vector containing the items.
 */
void freeItemVector(std::vector<File*> *vector_ptr)
{
    for(int i = 0; i < vector_ptr->size(); i++)
    {
        File* fp = vector_ptr->at(i);
        delete fp;
    }
}