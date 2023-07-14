
struct Asset_Catalog {
  char paths[16][64];
  s32 count;
};

global_var Asset_Catalog the_asset_catalog;

void asset_catalog_init(void) {
  the_asset_catalog = {};
}

void asset_catalog_add(char* path) {
  Asset_Catalog* catalog = &the_asset_catalog;
  
  char* src = path;
  char* dest = &catalog->paths[catalog->count][0];

  while(*src != '\0') {
    *dest = *src;
    dest += 1;
    src += 1;
  }
  
  *dest = '\0';
  
  catalog->count += 1;
}

void cstr_concat(char buff[], char* str) {
  char* dest = buff;
  while(*dest != '\0') dest += 1;
  
  char* src = str;
  while(*src != '\0') {
    *dest = *src;
    dest += 1;
    src += 1;
  }
  
  *dest = '\0';
}

void cstr_copy(char dest[], char src[]) {
  s32 at = 0;
  while(src[at] != '\0') {
    dest[at] = src[at];
    at += 1;
  }
  dest[at] = '\0';
}

b32 asset_catalog_find(char* file_name, char result[]) {
  Asset_Catalog* catalog = &the_asset_catalog;

  char buff[128] = {};
  char* ptr = buff;
  Loop(i, catalog->count) {
    buff[0] = '\0';
    cstr_concat(buff, catalog->paths[i]);
    cstr_concat(buff, "/");
    cstr_concat(buff, file_name);
    
    if(FileExists(buff)) {
      cstr_copy(result, buff);
      return true;
    }
  }
  
  return false;
}

Texture2D texture_asset_load(char* file_name) {
  Texture2D r = {};   
  char path[128];
  if(asset_catalog_find(file_name, path)) r = LoadTexture(path);
  
  return r;
}

Font font_asset_load(char* file_name, s32 font_size) {
  Font r = {};   
  char path[128];
  if(asset_catalog_find(file_name, path)) r = LoadFontEx(path, font_size, NULL, 0);
  
  return r;
}

