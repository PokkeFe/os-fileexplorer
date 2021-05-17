#include <iostream>
#include <algorithm>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <cstring>
#include <string.h>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#define WIDTH 800
#define HEIGHT 600

#define FILES_TOP_MARGIN 60
#define FILES_LEFT_MARGIN 20
#define FILE_HEIGHT 30

#define SCROLLBAR_X WIDTH - 15
#define SCROLLBAR_Y (FILES_TOP_MARGIN + 5)
#define SCROLLBAR_HEIGHT (HEIGHT - SCROLLBAR_Y - 5)
#define SCROLLBAR_HANDLE_RADIUS 5
const SDL_Color SCROLLBAR_COLOR = {0, 0, 0, 255};
const SDL_Color SCROLLBAR_HANDLE_COLOR = {200, 200, 200, 180};
const SDL_Color SCROLLBAR_HANDLE_DRAG_COLOR = {200, 200, 200, 255};

enum struct Type {
    DIRECTORY,
    EXECUTABLE,
    IMAGE,
    VIDEO,
    CODE,
    OTHER
};

// ! DEBUG FUNCTION
std::string typeToString(Type t)
{
    switch(t)
    {
        case Type::DIRECTORY:
            return "dir";
        case Type::EXECUTABLE:
            return "exe";
        case Type::IMAGE:
            return "img";
        case Type::VIDEO:
            return "vid";
        case Type::CODE:
            return "dev";
        case Type::OTHER:
            return "...";
    }
    return "ERR";
}

const std::vector<std::string> CODE_EXTENSIONS = {"h", "c", "cpp", "py", "java", "js"};
const std::vector<std::string> IMAGE_EXTENSIONS = {"jpg", "jpeg", "png", "tif", "tiff", "gif"};
const std::vector<std::string> VIDEO_EXTENSIONS = {"mp4", "mov", "mkv", "avi", "webm"};

class File {
    public:
        std::string name;
        bool is_dir;
        std::string extension;
        std::string permissions;
        std::string size;
        Type type;
};

typedef struct AppData {
    TTF_Font *font;

    // -- Files -- //
    std::vector<File*> files;

    SDL_Texture *Directory;
    SDL_Texture *Executable;
    SDL_Texture *Image;
    SDL_Texture *Video;
    SDL_Texture *Code;
    SDL_Texture *Other;
    SDL_Texture *Path;

    std::vector<SDL_Texture*> Text;
    std::vector<SDL_Texture*> Size;
    std::vector<SDL_Texture*> Permissions; 

    std::string PathText;

    SDL_Rect Path_rect;
    SDL_Rect Path_container;
    SDL_Rect Display_buffer;

    SDL_Rect Icon_rect;
    SDL_Rect Text_rect;
    SDL_Rect Size_rect;
    SDL_Rect Perm_rect;

    // Render Values -
    int page_height;
    int files_height;

    // Scrolling -
    int scroll_offset;
    float scrollbar_ratio;
    bool scrollbar_enabled;
    SDL_Rect scrollbar_guide_rect;
    SDL_Rect scrollbar_handle_rect;
    int scrollbar_click_xoff;
    int scrollbar_click_yoff;
    bool scrollbar_drag;

} AppData;

void initialize(SDL_Renderer *renderer, AppData *data);
void render(SDL_Renderer *renderer, AppData *data);

void resetRenderData(AppData *data);

std::vector<File*> getItemsInDirectory(std::string dirpath);
void freeItemVector(std::vector<File*> *vector_ptr);
std::string parsePermission(mode_t permission_mode);
std::string parseSize(size_t byte_size);
Type parseType(File* file);
bool doesContain(std::string str, std::vector<std::string> vec);

void setPath(AppData *data, std::string path);
void setFiles(SDL_Renderer *renderer, AppData *data, std::vector<File*> newFiles);
void renderFiles(SDL_Renderer *renderer, AppData *data);

void updateScrollbarRatio(AppData* data);
void updateScrollbarPosition(AppData* data, int mouse_y);
void renderScrollbar(SDL_Renderer* renderer, AppData* data);

void clickHandler(SDL_Event* event, SDL_Renderer* renderer, AppData* data);
void releaseHandler(SDL_Event* event, SDL_Renderer* renderer, AppData* data);
void motionHandler(SDL_Event* event, SDL_Renderer* renderer, AppData* data);


// ─── MAIN ───────────────────────────────────────────────────────────────────────


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

    // setup rendere
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Initializing AppData----------------------------------------------
    AppData data;
    setPath(&data, std::string(home));
    std::vector<File*> files = getItemsInDirectory(home);
    
    /*
    std::cout << "Hello!" << std::endl;
    setFiles(renderer, &data, files);
    std::cout << "Goodbye!" << std::endl;
    */
   
    data.page_height = HEIGHT - FILES_TOP_MARGIN;
    data.scroll_offset = 0;
    data.scrollbar_drag = false;

    // initialize and perform rendering loop
    initialize(renderer, &data);
    setFiles(renderer, &data, files);

    updateScrollbarRatio(&data);
    
    render(renderer, &data);
    SDL_Event event;
    SDL_WaitEvent(&event);
    while (event.type != SDL_QUIT)
    {
        // CLICK AND RELEASE HANDLING
        if (event.type == SDL_MOUSEBUTTONDOWN)
        {
            clickHandler(&event, renderer, &data);
        }
        else if (event.type == SDL_MOUSEBUTTONUP)
        {
            releaseHandler(&event, renderer, &data);
        }

        // DRAG HANDLING
        if(event.type == SDL_MOUSEMOTION)
        {
            motionHandler(&event, renderer, &data);
        } else

        // MOUSE WHEEL HANDLING
        if (event.type == SDL_MOUSEWHEEL);
        {
            if(data.scrollbar_enabled && event.wheel.y <= 5 && event.wheel.y >= -5) {
                data.scroll_offset -= event.wheel.y << 4;
                // Don't allow to scroll above files (offset inverted)
                if(data.scroll_offset < 0) data.scroll_offset = 0;
                if(data.scroll_offset > (data.files_height - data.page_height)) data.scroll_offset = (data.files_height - data.page_height);
            }
        }

        resetRenderData(&data);
        render(renderer, &data);
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


// ─── INIT & RENDER ──────────────────────────────────────────────────────────────


// Reset any data needed for a render update
void resetRenderData(AppData *data) {
    data->Icon_rect = {FILES_LEFT_MARGIN, FILES_TOP_MARGIN - data->scroll_offset, 30, 30};
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
    
    /*-----------------------Initializing Text Textures------------------------*/
    data->font = TTF_OpenFont("resrc/OpenSans-Regular.ttf", 12);
    SDL_Color color = {0, 0, 0};
    SDL_Surface *path_surface = TTF_RenderText_Solid(data->font, data->PathText.c_str(), color);
    data->Path = SDL_CreateTextureFromSurface(renderer, path_surface);
    SDL_FreeSurface(path_surface);

    int j;
    for(j = 0; j < data->files.size(); j++){

        // -- File Name -- //
        SDL_Color color = {0, 0, 0};
        SDL_Surface *text_surface = TTF_RenderText_Solid(data->font, data->files[j]->name.c_str(), color);
        data->Text.push_back(SDL_CreateTextureFromSurface(renderer, text_surface));
        SDL_FreeSurface(text_surface);

        // -- File Size -- //
        SDL_Surface *text_surface2 = TTF_RenderText_Solid(data->font, data->files[j]->size.c_str(), color);
        data->Size.push_back(SDL_CreateTextureFromSurface(renderer, text_surface2));
        SDL_FreeSurface(text_surface2);

        // -- File Permissions -- //
        SDL_Surface *text_surface3 = TTF_RenderText_Solid(data->font, data->files[j]->permissions.c_str(), color);
        data->Permissions.push_back(SDL_CreateTextureFromSurface(renderer, text_surface3));
        SDL_FreeSurface(text_surface3);
    }

    /*-----------------------Initializing Rectangles----------------------*/
    // My_rect = {x, y, width, height}

    data->Path_container = {10, 10, 780, 40};
    data->Display_buffer = {10, 0, 800, 50};

    data->Path_rect = {20, 21, 50, 50};
    SDL_QueryTexture(data->Path, NULL, NULL, &(data->Path_rect.w), &(data->Path_rect.h));
    
    data->Icon_rect = {20, 60, 30, 30};  

    data->scrollbar_guide_rect = {SCROLLBAR_X, SCROLLBAR_Y, 1, SCROLLBAR_HEIGHT};
}

void render(SDL_Renderer *renderer, AppData *data){
    // erase renderer content
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(renderer);
    
    // TODO: draw!

    // -- Render Files -- //
    renderFiles(renderer, data);

    // -- Display Buffer -- //
    SDL_RenderDrawRect(renderer, &(data->Display_buffer));
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderFillRect(renderer, &(data->Display_buffer));

    // -- Path Display -- //
    SDL_RenderDrawRect(renderer, &(data->Path_container));
    SDL_SetRenderDrawColor(renderer, 0xd9, 0xdb, 0xb9, 0xFF);
    SDL_RenderFillRect(renderer, &(data->Path_container));
    SDL_QueryTexture(data->Path, NULL, NULL, &(data->Path_rect.w), &(data->Path_rect.h));
    SDL_RenderCopy(renderer, data->Path, NULL, &(data->Path_rect));

    // -- Render Scroll Bar -- //
    renderScrollbar(renderer, data);

    // show rendered frame
    SDL_RenderPresent(renderer);
}


// ─── FILES ──────────────────────────────────────────────────────────────────────


/** Get all the file/directory items at the given dirpath
 * @param dirpath Path of the directory to get the contents of.
 * @return A vector of files and folders inside the directory.
 */
std::vector<File*> getItemsInDirectory(std::string dirpath)
{
    if(dirpath == "") dirpath = "/";
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

        int first_file = 0;
        int search_index, search_end;
        
        while((entry = readdir(dir)) != NULL)
        {
            File* file_entry = new File();
            file_entry->name = entry->d_name;
            if(file_entry->name == ".") continue;
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
            } else {
                file_entry->extension = "";
            }

            // extract permissions
            file_entry->permissions = parsePermission(entry_info.st_mode);
            
            // extract size
            file_entry->size = parseSize(entry_info.st_size);

            // extract type
            file_entry->type = parseType(file_entry);

            //printf("%-40s - %s\n", file_entry->name.c_str(), typeToString(file_entry->type).c_str());

            // smarter adding
            if(file_entry->is_dir)
            {
                search_index = 0;
                search_end = first_file;
            }
            else
            {
                search_index = first_file;
                search_end = file_vector.size();
            }

            while(search_index < search_end)
            {
                // compare the name strings. if new < index, break
                if(file_entry->name < file_vector.at(search_index)->name) break;
                search_index++;
            }
            
            auto it = file_vector.begin() + search_index;
            file_vector.insert(it, file_entry);
            if(file_entry->is_dir) first_file++;
        }
    }
    else
    {
        printf("Error: %s\n", strerror(errno));
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

/** Takes a pointer to a file and returns the type of the file
 * @param file File to get type of
 * @return The type of the file
 */
Type parseType(File* file) 
{
    // Directory
    if(file->is_dir) return Type::DIRECTORY;
    // Executable
    if(file->permissions.find('x') != std::string::npos) return Type::EXECUTABLE;
    // Code file
    if(doesContain(file->extension, CODE_EXTENSIONS)) return Type::CODE;
    // Image
    if(doesContain(file->extension, IMAGE_EXTENSIONS)) return Type::IMAGE;
    // Video
    if(doesContain(file->extension, VIDEO_EXTENSIONS)) return Type::VIDEO;
    // Other
    return Type::OTHER;
}

/** Checks if a string vector contains an instance of given string
 * @param str string to search for
 * @param vec vector to search
 * @return True if vectors contains string
 */
bool doesContain(std::string str, std::vector<std::string> vec)
{
    for(int i = 0; i < vec.size(); i++)
    {
        if(vec[i] == str) return true;
    }
    return false;
}

/** Sets the path text for the current file directory path
 * @param data App Data used in rendering main-stage content
 * @param path Path to change the current PathText into
 */
void setPath(AppData *data, std::string path){
    data->PathText = path;
}

/** Sets the path text for the current file directory path
 * @param data App Data used in rendering main-stage content
 * @param newFiles Files to change the current Files into
 */
void setFiles(SDL_Renderer *renderer, AppData *data, std::vector<File*> newFiles){
    data->files.clear();
    data->files = newFiles;
    data->Text.clear();
    data->Size.clear();
    data->Permissions.clear();

    int j;
    for(j = 0; j < data->files.size(); j++){

        // -- File Name -- //
        SDL_Color color = {0, 0, 0};
        SDL_Surface *text_surface = TTF_RenderText_Solid(data->font, data->files[j]->name.c_str(), color);
        data->Text.push_back(SDL_CreateTextureFromSurface(renderer, text_surface));
        SDL_FreeSurface(text_surface);

        // -- File Size -- //
        SDL_Surface *text_surface2 = TTF_RenderText_Solid(data->font, data->files[j]->size.c_str(), color);
        data->Size.push_back(SDL_CreateTextureFromSurface(renderer, text_surface2));
        SDL_FreeSurface(text_surface2);

        // -- File Permissions -- //
        SDL_Surface *text_surface3 = TTF_RenderText_Solid(data->font, data->files[j]->permissions.c_str(), color);
        data->Permissions.push_back(SDL_CreateTextureFromSurface(renderer, text_surface3));
        SDL_FreeSurface(text_surface3);
    }
}

/** Render all files/Icons/Size/permissions within a current path directory
 * @param renderer Main-stage renderer
 * @param data App Data used in rendering main-state content
 * @param files list of files within the current path directory
 */
void renderFiles(SDL_Renderer *renderer, AppData *data){

    int i; 
    for(i = 0; i < data->files.size(); i++){

        // ----Render Icon---- //

        switch(data->files[i]->type)
        {
            case Type::DIRECTORY:
                SDL_RenderCopy(renderer, data->Directory, NULL, &(data->Icon_rect));
                break;
            case Type::EXECUTABLE:
                SDL_RenderCopy(renderer, data->Executable, NULL, &(data->Icon_rect));
                break;
            case Type::IMAGE:
                SDL_RenderCopy(renderer, data->Image, NULL, &(data->Icon_rect));
                break;
            case Type::VIDEO:
                SDL_RenderCopy(renderer, data->Video, NULL, &(data->Icon_rect));
                break;
            case Type::CODE:
                SDL_RenderCopy(renderer, data->Code, NULL, &(data->Icon_rect));
                break;
            case Type::OTHER:
                SDL_RenderCopy(renderer, data->Other, NULL, &(data->Icon_rect));
                break;
        }
        
        // ----Render Text---- //
        
        data->Text_rect.x = data->Icon_rect.x + 40;
        data->Text_rect.y = data->Icon_rect.y + 9;
        SDL_QueryTexture(data->Text[i], NULL, NULL, &(data->Text_rect.w), &(data->Text_rect.h));
        SDL_RenderCopy(renderer, data->Text[i], NULL, &(data->Text_rect));

        // ----Render Size---- //

        data->Size_rect.x = data->Text_rect.x + 300;
        data->Size_rect.y = data->Icon_rect.y + 9;
        if(!data->files[i]->is_dir) {
            SDL_QueryTexture(data->Size[i], NULL, NULL, &(data->Size_rect.w), &(data->Size_rect.h));
            SDL_RenderCopy(renderer, data->Size[i], NULL, &(data->Size_rect));
        }

        // ----Render Permissions---- //

        data->Perm_rect.x = data->Text_rect.x + 400;
        data->Perm_rect.y = data->Icon_rect.y + 9;
        SDL_QueryTexture(data->Permissions[i], NULL, NULL, &(data->Perm_rect.w), &(data->Perm_rect.h));
        SDL_RenderCopy(renderer, data->Permissions[i], NULL, &(data->Perm_rect));

        // ----Increment Height---- //

        data->Icon_rect.y += 30;
    }
}


// ─── SCROLLBAR ──────────────────────────────────────────────────────────────────


/** Updates the scrollbar after a change in the number of files visible on screen
 * @param data AppData
 * @param num_files number of files on screen
 */
void updateScrollbarRatio(AppData* data)
{
    int num_files = data->files.size();
    data->files_height = num_files * FILE_HEIGHT;
    data->scrollbar_ratio = (float) data->page_height / (float) data->files_height;
    if(data->scrollbar_ratio >= 1.0f)
    {
        data->scrollbar_enabled = false;
    } else {
        data->scrollbar_enabled = true;
    }
}

/** Updates the scrollbar's position based on the mouse's location
 */
void updateScrollbarPosition(AppData* data, int mouse_y)
{
    // get normalized y
    int y = mouse_y - data->scrollbar_click_yoff - SCROLLBAR_Y;
    if(y < 0) y = 0;
    
    float handle_offset_ratio = (float) y / (float) (SCROLLBAR_HEIGHT - data->scrollbar_handle_rect.h);
    if(handle_offset_ratio > 1.0f) handle_offset_ratio = 1.0f;

    data->scroll_offset = (int) (handle_offset_ratio * (data->files_height - data->page_height));
}

/** Renders the scroll bar
 */
void renderScrollbar(SDL_Renderer* renderer, AppData* data)
{
    // only render if scroll height is less than 1.0
    if(data->scrollbar_enabled)
    {
        // render line
        SDL_SetRenderDrawColor(renderer, SCROLLBAR_COLOR.r, SCROLLBAR_COLOR.g, SCROLLBAR_COLOR.b, SCROLLBAR_COLOR.a);
        SDL_RenderDrawRect(renderer, &data->scrollbar_guide_rect);

        // render grab box
        if(data->scrollbar_drag)
        {
            SDL_SetRenderDrawColor(renderer, SCROLLBAR_HANDLE_DRAG_COLOR.r, SCROLLBAR_HANDLE_DRAG_COLOR.g, SCROLLBAR_HANDLE_DRAG_COLOR.b, SCROLLBAR_HANDLE_DRAG_COLOR.a);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, SCROLLBAR_HANDLE_COLOR.r, SCROLLBAR_HANDLE_COLOR.g, SCROLLBAR_HANDLE_COLOR.b, SCROLLBAR_HANDLE_COLOR.a);
        }
        // create rectangle
        int handle_height = (int) ((float)SCROLLBAR_HEIGHT * data->scrollbar_ratio);
        float handle_offset_ratio = (float) data->scroll_offset / (float) (data->files_height - data->page_height);
        data->scrollbar_handle_rect = {SCROLLBAR_X - SCROLLBAR_HANDLE_RADIUS, SCROLLBAR_Y + (int)((float)(SCROLLBAR_HEIGHT - handle_height) * handle_offset_ratio), SCROLLBAR_HANDLE_RADIUS << 1, (int) ((float)SCROLLBAR_HEIGHT * data->scrollbar_ratio)};
        SDL_RenderFillRect(renderer, &data->scrollbar_handle_rect);
    }
}


// ─── MOUSE ──────────────────────────────────────────────────────────────────────


/** Handle any logic for mouse click events
 */
void clickHandler(SDL_Event* event, SDL_Renderer* renderer, AppData* data) 
{
    int click_y = event->button.y;
    int click_x = event->button.x;
    // First, check if the click happened in the header section, above the files
    if(click_y < FILES_TOP_MARGIN)
    {
        // HANDLE HEADER CLICKS
    }
    // Second, check if the click happened near the scrollbar
    else if(click_x >= SCROLLBAR_X - SCROLLBAR_HANDLE_RADIUS - 5)
    {
        // HANDLE SCROLLBAR CLICKS
        // check that the click lands within scrollbar handle boundaries
        if(
            click_x >= data->scrollbar_handle_rect.x && click_x <= data->scrollbar_handle_rect.x + data->scrollbar_handle_rect.w &&
            click_y >= data->scrollbar_handle_rect.y && click_y <= data->scrollbar_handle_rect.y + data->scrollbar_handle_rect.h
        )
        {
            // set the click offset
            data->scrollbar_click_xoff = click_x - data->scrollbar_handle_rect.x;
            data->scrollbar_click_yoff = click_y - data->scrollbar_handle_rect.y;
            // set data->scrollbar_drag to true
            data->scrollbar_drag = true;
        }

    }
    // If none of the above, try to find the file at that y location
    else
    {
        // HANDLE FILE CLICKS
        int y_normalized = click_y - FILES_TOP_MARGIN + data->scroll_offset;
        int file_index = y_normalized / FILE_HEIGHT;
        if(file_index < data->files.size())
        {
            // CLICKED FILE
            //printf("Clicked %s\n", data->files.at(file_index)->name.c_str());

            // Directory Change
            std::string fullPath;
            if(data->files.at(file_index)->is_dir == true){
                if(data->files.at(file_index)->name == ".."){
                    std::size_t target = data->PathText.find_last_of("/");
                    fullPath = data->PathText.substr(0, target);
                    setPath(data, fullPath);
                    std::vector<File*> newFiles = getItemsInDirectory(fullPath);
                    setFiles(renderer, data, newFiles);
                    updateScrollbarRatio(data);
                    data->scroll_offset = 0;
                    renderScrollbar(renderer, data);
                    SDL_Color color = {0, 0, 0};
                    SDL_Surface *path_surface = TTF_RenderText_Solid(data->font, data->PathText.c_str(), color);
                    data->Path = SDL_CreateTextureFromSurface(renderer, path_surface);
                    SDL_FreeSurface(path_surface);
                    std::cout << "New Path: " << data->PathText << std::endl;
                }
                else{
                    fullPath = data->PathText + "/" + data->files.at(file_index)->name;
                    setPath(data, fullPath);
                    std::vector<File*> newFiles = getItemsInDirectory(fullPath);
                    setFiles(renderer, data, newFiles);
                    updateScrollbarRatio(data);
                    data->scroll_offset = 0;
                    renderScrollbar(renderer, data);
                    SDL_Color color = {0, 0, 0};
                    SDL_Surface *path_surface = TTF_RenderText_Solid(data->font, data->PathText.c_str(), color);
                    data->Path = SDL_CreateTextureFromSurface(renderer, path_surface);
                    SDL_FreeSurface(path_surface);
                    std::cout << "New Path: " << data->PathText << std::endl;
                }             
            }

            // Execute Program
            else{
                int pid = fork();
                if(pid == 0){
                    fullPath = data->PathText + "/" + data->files.at(file_index)->name;
                    char *pathArray = &fullPath[0];
                    char openCommand[] = "xdg-open";
                    char *const executionArguments[3] = {openCommand, pathArray, NULL};
                    execv("/usr/bin/xdg-open", executionArguments);
                }
            }
        }
        
    }
}

/** Handle any logic for mouse release events
 */
void releaseHandler(SDL_Event* event, SDL_Renderer* renderer, AppData* data)
{
    if(data->scrollbar_drag)
    {
        data->scrollbar_drag = false;
    }
}

/** Handle any logic for mouse movement events
 */
void motionHandler(SDL_Event* event, SDL_Renderer* renderer, AppData* data)
{
    // If scrollbar is being dragged, update scrollbar position
    if(data->scrollbar_drag)
    {
        updateScrollbarPosition(data, event->motion.y);
        return;
    }
}