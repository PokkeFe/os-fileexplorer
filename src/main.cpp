#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
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

typedef struct AppData {
    TTF_Font *font;
    SDL_Texture *Directory;
    SDL_Texture *Executable;
    SDL_Texture *Image;
    SDL_Texture *Video;
    SDL_Texture *Code;
    SDL_Texture *Other;
    std::string PathText;
    SDL_Texture *Path;
    SDL_Rect Icon_rect;
    SDL_Rect Path_rect;
    SDL_Rect Path_container;
} AppData;

void initialize(SDL_Renderer *renderer, AppData *data);
void render(SDL_Renderer *renderer, AppData *data);

std::vector<File*> getItemsInDirectory(std::string dirpath);
void freeItemVector(std::vector<File*> *vector_ptr);
void setPath(AppData *data, std::string path);

int main(int argc, char **argv)
{
    char *home = getenv("HOME");
    printf("HOME: %s\n", home);

    // initializing SDL as Video
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();

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


    // Initializing AppData----------------------------------------------
    AppData data;
    setPath(&data, std::string(home));

    // initialize and perform rendering loop
    initialize(renderer, &data);
    render(renderer, &data);
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
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}

void initialize(SDL_Renderer *renderer, AppData *data)
{
    // set color of background when erasing frame
    SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255);

    /*-----------------------Initializing Textures-----------------------*/
    SDL_Surface *img_surf = IMG_Load("resrc/File Icons/DirectoryIcon.png");
    data->Directory = SDL_CreateTextureFromSurface(renderer, img_surf);
    SDL_FreeSurface(img_surf);

    SDL_Surface *img2_surf = IMG_Load("resrc/File Icons/ExeIcon.png");
    data->Executable = SDL_CreateTextureFromSurface(renderer, img2_surf);
    SDL_FreeSurface(img2_surf);
    
    SDL_Surface *img3_surf = IMG_Load("resrc/File Icons/ImageIcon.png");
    data->Image = SDL_CreateTextureFromSurface(renderer, img3_surf);
    SDL_FreeSurface(img3_surf);
    
    SDL_Surface *img4_surf = IMG_Load("resrc/File Icons/VideoIcon.png");
    data->Video = SDL_CreateTextureFromSurface(renderer, img4_surf);
    SDL_FreeSurface(img4_surf);
    
    SDL_Surface *img5_surf = IMG_Load("resrc/File Icons/CodeIcon.png");
    data->Code = SDL_CreateTextureFromSurface(renderer, img5_surf);
    SDL_FreeSurface(img5_surf);
    
    SDL_Surface *img6_surf = IMG_Load("resrc/File Icons/OtherIcon.png");
    data->Other = SDL_CreateTextureFromSurface(renderer, img6_surf);
    SDL_FreeSurface(img6_surf);
    
    /*-----------------------Initializing Path Text------------------------*/
    data->font = TTF_OpenFont("resrc/OpenSans-Regular.ttf", 24);
    SDL_Color color = {0, 0, 0};
    SDL_Surface *path_surface = TTF_RenderText_Solid(data->font, data->PathText.c_str(), color);
    data->Path = SDL_CreateTextureFromSurface(renderer, path_surface);
    SDL_FreeSurface(path_surface);


    /*-----------------------Initializing Rectangles----------------------*/
    // My_rect = {x, y, width, height}
    
    data->Path_container = {30, 30, 200, 150};

    data->Path_rect = {50, 50, 100, 100};
    SDL_QueryTexture(data->Path, NULL, NULL, &(data->Path_rect.w), &(data->Path_rect.h));
    
    data->Icon_rect = {50, 250, 50, 50};
}

void render(SDL_Renderer *renderer, AppData *data){
    // erase renderer content
    SDL_RenderClear(renderer);
    
    // TODO: draw!
    SDL_RenderCopy(renderer, data->Path, NULL, &(data->Path_rect));
    SDL_RenderCopy(renderer, data->Other, NULL, &(data->Icon_rect));

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

void setPath(AppData *data, std::string path){
    data->PathText = path;
}