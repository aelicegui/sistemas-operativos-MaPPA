#include "../include/estructuras_principales.h"
t_FAT* fat;
void* bloques;

void iniciarFAT(){
    int cantEntradas = cant_bloques_total() - cant_bloques_swap();

    for (int i = 0; i < (cantEntradas); i++) {
        fat[i] = (uint32_t) 0;
    }

    char* path = path_fat();

    FILE *archivoFAT = fopen(path, "wb");
    if(archivoFAT==NULL){
        return;
    }
    fwrite(fat, sizeof(uint32_t), cantEntradas, archivoFAT);

    log_info(filesystem, "escribe archivo");

    fclose(archivoFAT);
}

int levantarFAT(){
    char* path = path_fat();
    FILE *archivoFAT = fopen(path, "rb");
    int cantEntradas = cant_bloques_total() - cant_bloques_swap();
    fat = malloc((cantEntradas) * sizeof(uint32_t));

    if(archivoFAT == NULL){
        iniciarFAT();
        return 1;
    }
    else{
        int cantEntradas = cant_bloques_total() - cant_bloques_swap();
        fread(fat, sizeof(uint32_t), cantEntradas, archivoFAT);
    }
    
    fclose(archivoFAT);
    return 1;
}

void actualizarFAT(){
    FILE *archivoFAT = fopen(path_fat(), "wb");
    int cantEntradas = cant_bloques_total() - cant_bloques_swap();
    fwrite(fat, sizeof(uint32_t), cantEntradas, archivoFAT);
    fclose(archivoFAT);
    return;
}


void liberarFAT() {
    free(fat);
}

t_FCB* buscarFCB(char* nombreArchivo){
    t_FCB* fcb; 
    int cant_fcbs = list_size(fcbs);
    for(int i = 0; i<cant_fcbs;i++){
        fcb = list_get(fcbs, i);
        if(strcmp(fcb->NOMBRE_ARCHIVO, nombreArchivo) == 0){
            return fcb;
        }
    }

    return buscarFCBArchivo(nombreArchivo);
}

t_FCB* buscarFCBArchivo(char* nombreArchivo){
    t_FCB* fcb = malloc(sizeof(t_FCB));
    char * extencion = "fcb";
    char *path = string_new();
    string_append_with_format(&path, "%s/%s.%s", path_fcb(), nombreArchivo, extencion);
    t_config *archivoFCB = config_create(path);
    if(archivoFCB == NULL){
        //no se encontro el archivo
        free(fcb);
        return NULL;
    }
    else{
        fcb->NOMBRE_ARCHIVO = config_get_string_value(archivoFCB, "NOMBRE_ARCHIVO");
        fcb->TAMANIO_ARCHIVO =  config_get_int_value(archivoFCB, "TAMANIO_ARCHIVO");
        fcb->BLOQUE_INICIAL =  config_get_int_value(archivoFCB, "BLOQUE_INICIAL");
        fcb->CONFIG = archivoFCB;
        list_add(fcbs, fcb);
        return fcb;
    }
}

void iniciarBLOQUES(){
    char* path = path_bloques();
    int cant_bloques = cant_bloques_total();
    int cant_bloques_swapp = cant_bloques_swap();
    int tamanio_bloque = tam_bloque();

    for (int i = 0; i < (cant_bloques_swapp); i++) {
        memset(bloques+i*tamanio_bloque*sizeof(char),'\244',sizeof(char));
    }
    for (int i = cant_bloques_swapp; i < (cant_bloques); i++) {
        memset(bloques+i*tamanio_bloque*sizeof(char),'\0',sizeof(char));
    }

    FILE *archivoBloques = fopen(path, "wb");
    if(archivoBloques==NULL){
        free(bloques);
        return;
    }
    fwrite(bloques, tamanio_bloque*sizeof(char), cant_bloques, archivoBloques);

    fclose(archivoBloques);
}

int levantarBLOQUES(){
    int cant_bloques = cant_bloques_total();
    int tamanio_bloque = tam_bloque();
    bloques = malloc(cant_bloques * tamanio_bloque * sizeof(char));
    char* path = path_bloques();
    FILE *archivoBloques = fopen(path, "rb");

    if(archivoBloques == NULL){
        //no se encontro el archivo
        iniciarBLOQUES();
        return 1;
    }
    else{
        
        fread(bloques, sizeof(char)*tamanio_bloque, cant_bloques, archivoBloques);
        fclose(archivoBloques);
    }
    return 1;
}
void actualizarBLOQUES(){
    int cant_bloques = cant_bloques_total();

    FILE *archivoBloques = fopen(path_bloques(), "wb");

    if(archivoBloques == NULL){
    }
    else{
        int tamanio_bloque = tam_bloque();
        fwrite(bloques, tamanio_bloque, cant_bloques, archivoBloques);
        fclose(archivoBloques);
    }
}

//a evualuar el tema del sizeof
void liberarBloques() {

    free(bloques);

}

//int abrir_archivo(char* nombreArchivo, )

void log_crear_archivo(char* nombre){
    log_info(logger_fs, "Crear Archivo: %s",nombre);
}

void log_abrir_archivo(char* nombre){
    log_info(logger_fs, "Abrir Archivo: %s",nombre);
}

void log_truncar_archivo(char* nombre, int tamanio){
    log_info(logger_fs, "Truncar Archivo: %s - Tamaño: %d",nombre,tamanio);
}

void log_leer_archivo(char* nombre, int puntero, int direccion){
    log_info(logger_fs, "Leer Archivo: %s - Puntero: %d - Memoria: %d",nombre,puntero,direccion);
}

void log_escribir_archivo(char* nombre, int puntero, int direccion){
    log_info(logger_fs, "Escribir Archivo: %s - Puntero: %d - Memoria: %d",nombre,puntero,direccion);
}

void log_acceso_FAT(int nro_entrada, uint32_t valor){
    usleep(retardo_acceso_fat()*1000);
    log_info(logger_fs, "Acceso FAT - Entrada: %d - Valor: %d",nro_entrada, valor);
}

void log_acceso_Bloque_Archivo(char* nombre,int nro_bloque_arch, int nro_bloque_fs){
    usleep(retardo_acceso_bloque()*1000);
    log_info(logger_fs, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque FS: %d",nombre,nro_bloque_arch,nro_bloque_fs);
}

void log_acceso_Bloque_SWAP(int nro_bloque){
    usleep(retardo_acceso_bloque()*1000);
    log_info(logger_fs, "Acceso SWAP: %d",nro_bloque);
}

