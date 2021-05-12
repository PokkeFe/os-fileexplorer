#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <cstring>
#include <string.h>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>

#define WIDTH 800
#define HEIGHT 600

class File {
    public:
        std::string name;
        bool is_dir;
        std::string extension;
        std::string permissions;
        std::string size;
};

typedef struct AppData {
    TTF_Font *font;

    SDL_Texture *Directory;
    SDL_Texture *Executable;
    SDL_Texture *Image;
    SDL_Texture *Video;
    SDL_Texture *Code;
    SDL_Texture *Other;
    SDL_Texture *Text;
    SDL_Texture *Path;

    std::string PathText;

    SDL_Rect Icon_rect;
    SDL_Rect Text_rect;
    SDL_Rect Path_rect;
    SDL_Rect Path_container;

    int scroll_offset;
} AppData;

void initialize(SDL_Renderer *renderer, AppData *data);
void render(SDL_Renderer *renderer, AppData *data, std::vector<File*> files);

void resetRenderData(AppData *data);

std::vector<File*> getItemsInDirectory(std::string dirpath);
void freeItemVector(std::vector<File*> *vector_ptr);
std::string parsePermission(mode_t permission_mode);
std::string parseSize(size_t byte_size);

void setPath(AppData *data, std::string path);
void renderFiles(SDL_Renderer *renderer, AppData *data, std::vector<File*> files);

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

    // Initializing AppData----------------------------------------------
    AppData data;
    setPath(&data, std::string(home));
    std::vector<File*> files = getItemsInDirectory(home);
    data.scroll_offset = 0;

    // initialize and perform rendering loop
    initialize(renderer, &data);
    render(renderer, &data, files);
    SDL_Event event;
    SDL_WaitEvent(&event);
    while (event.type != SDL_QUIT)
    {
        if (event.type == SDL_MOUSEWHEEL)
        {
            data.scroll_offset += event.wheel.y << 4;
            // Don't allow to scroll above files (offset inverted)
            if(data.scroll_offset > 0) data.scroll_offset = 0;
        }

        resetRenderData(&data);
        render(renderer, &data, files);
        SDL_WaitEvent(&event);
    }

    // clean up
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    freeItemVector(&files);

    return 0;
}

// Reset any data needed for a render update
void resetRenderData(AppData *data) {
    data->Icon_rect = {20, 60 + data->scroll_offset, 30, 30};
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
    data->font = TTF_OpenFont("resrc/OpenSans-Regular.ttf", 12);
    SDL_Color color = {0, 0, 0};
    SDL_Surface *path_surface = TTF_RenderText_Solid(data->font, data->PathText.c_str(), color);
    data->Path = SDL_CreateTextureFromSurface(renderer, path_surface);
    SDL_FreeSurface(path_surface);


    /*-----------------------Initializing Rectangles----------------------*/
    // My_rect = {x, y, width, height}

    data->Path_container = {10, 10, 780, 40};

    data->Path_rect = {20, 21, 50, 50};
    SDL_QueryTexture(data->Path, NULL, NULL, &(data->Path_rect.w), &(data->Path_rect.h));
    
    data->Icon_rect = {20, 60, 30, 30};
}

void render(SDL_Renderer *renderer, AppData *data, std::vector<File*> files){
    // erase renderer content
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0);
    SDL_RenderClear(renderer);
    
    // TODO: draw!
    SDL_RenderDrawRect(renderer, &(data->Path_container));
    SDL_SetRenderDrawColor(renderer, 0xd9, 0xdb, 0xb9, 0x00);
    SDL_RenderFillRect(renderer, &(data->Path_container));
    SDL_RenderCopy(renderer, data->Path, NULL, &(data->Path_rect));
    //SDL_RenderCopy(renderer, data->Other, NULL, &(data->Icon_rect));

    // -- Render Files --
    renderFiles(renderer, data, files);

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
    struct stat dir_info;
    int err = stat(dirpath.c_str(), &dir_info);
    if(err == 0 && S_ISDIR(dir_info.st_mode))
    {
        DIR* dir = opendir(dirpath.c_str());
        struct dirent *entry;
        struct stat entry_info;
        std::string full_path;

        int dot_pos;
        
        while((entry = readdir(dir)) != NULL)
        {
            File* file_entry = new File();
            file_entry->name = entry->d_name;
            file_entry->is_dir = (entry->d_type == DT_DIR);
            // get file stat
            full_path = dirpath + "/" + file_entry->name;
            stat(full_path.c_str(), &entry_info);
            // extract extension
            // if a . is found
            if((dot_pos = file_entry->name.find_last_of('.')) != file_entry->name.npos) {
                // set the extension to every character after the .
                file_entry->extension = file_entry->name;
                file_entry->extension = file_entry->extension.erase(0, dot_pos+1);
            }

            // extract permissions
            file_entry->permissions = parsePermission(entry_info.st_mode);
            
            // extract size
            file_entry->size = parseSize(entry_info.st_size);

            //printf("%-40s - %s\n", file_entry->name.c_str(), file_entry->size.c_str());
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

/** Parses permissions mode into permission string
 * @param permission_mode mode_t permission format
 * @return A string of permissions
 */
std::string parsePermission(mode_t permission_mode)
{
    std::string permission_string = "";
    // if it's a d, push d
    if(S_ISDIR(permission_mode))
    {
        permission_string.push_back('d');
    } else {
        permission_string.push_back('-');
    }

    int shift_root;
    for(int i = 2; i >= 0; i--) {
        shift_root = i * 3;
        if (permission_mode & (0x1 << (shift_root + 2)))
        {
            permission_string.push_back('r');
        } else {
            permission_string.push_back('-');
        }
        if (permission_mode & (0x1 << (shift_root + 1)))
        {
            permission_string.push_back('w');
        } else {
            permission_string.push_back('-');
        }
        if (permission_mode & (0x1 << (shift_root)))
        {
            permission_string.push_back('x');
        } else {
            permission_string.push_back('-');
        }
    }
    return permission_string;
}

/** Parse size in bytes into human readable format
 * @param byte_size size of file in bytes
 * @return Human readable size string
 */
std::string parseSize(size_t byte_size) {
    std::string size_str = "";
    int den, pre, pos;
    
    // GiB
    if(byte_size >> 30) {
        den = 1 << 30;
        pre = byte_size / den;
        pos = (byte_size % den) / (den / 10);
        size_str.append(std::to_string(pre));
        if(pos != 0)
        {
            size_str.push_back('.');
            size_str.append(std::to_string(pos));
        }
        size_str.append(" GiB");
    }
    // MiB
    else if (byte_size >> 20) {
        den = 1 << 20;
        pre = byte_size / den;
        pos = (byte_size % den) / (den / 10);
        size_str.append(std::to_string(pre));
        if(pos != 0)
        {
            size_str.push_back('.');
            size_str.append(std::to_string(pos));
        }
        size_str.append(" MiB");
    }
    // KiB
    else if (byte_size >> 10) {
        den = 1 << 10;
        pre = byte_size / den;
        pos = (byte_size % den) / (den / 10);
        size_str.append(std::to_string(pre));
        if(pos != 0)
        {
            size_str.push_back('.');
            size_str.append(std::to_string(pos));
        }
        size_str.append(" KiB");
    }
    // B
    else {
        size_str.append(std::to_string(byte_size));
        size_str.append(" B");
    }

    return size_str;
}

void setPath(AppData *data, std::string path){
    data->PathText = path;
}

void renderFiles(SDL_Renderer *renderer, AppData *data, std::vector<File*> files){

    int i; 
    for(i = 0; i < files.size(); i++){

        // ----Render Icon---- //
        if(files[i]->is_dir == true){
            SDL_RenderCopy(renderer, data->Directory, NULL, &(data->Icon_rect));
        }
        else{
            SDL_RenderCopy(renderer, data->Other, NULL, &(data->Icon_rect));
        }
        
        // ----Render Text---- //
        SDL_Color color = {0, 0, 0};
        SDL_Surface *text_surface = TTF_RenderText_Solid(data->font, files[i]->name.c_str(), color);
        data->Text = SDL_CreateTextureFromSurface(renderer, text_surface);
        SDL_FreeSurface(text_surface);
        data->Text_rect.x = data->Icon_rect.x + 40;
        data->Text_rect.y = data->Icon_rect.y + 9;
        SDL_QueryTexture(data->Text, NULL, NULL, &(data->Text_rect.w), &(data->Text_rect.h));
        SDL_RenderCopy(renderer, data->Text, NULL, &(data->Text_rect));

        // ----Increment Height---- //
        data->Icon_rect.y += 30;
    }
}