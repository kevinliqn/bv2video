#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "dirent.h" // ʹ��ǰ���ṩ��dirent.hʵ��
#include "cJSON.h"
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <locale.h>

// ��������ļ������Ƴ��Ⱥͳ�ʼ�����С
#define MAX_NAME_LEN 256
#define INITIAL_SIZE 10

// ��̬����ṹ
typedef struct {
    char** names;
    int size;
    int capacity;
} DynamicArray;
// ��ʼ����̬����
DynamicArray* createArray(int capacity) {
    DynamicArray* array = (DynamicArray*)malloc(sizeof(DynamicArray));
    array->names = (char**)malloc(capacity * sizeof(char*));
    array->size = 0;
    array->capacity = capacity;
    return array;
}

// ����ļ������Ƶ���̬������
void addName(DynamicArray* array, const char* name) {
    if (array->size == array->capacity) {
        char** temp = (char**)realloc(array->names, array->capacity * 2 * sizeof(char*));
        if (temp == NULL) {
            // �ڴ����ʧ�ܣ��������
            fprintf(stderr, "�ڴ����ʧ��\n");
            return;
        }
        array->names = temp;
        array->capacity *= 2;
    }
    array->names[array->size] = _strdup(name);
    array->size++;
}

// �ͷŶ�̬����
void freeArray(DynamicArray* array) {
    for (int i = 0; i < array->size; i++) {
        free(array->names[i]);
    }
    free(array->names);
    free(array);
}
// ��ȡ�ļ�����
char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("�޷����ļ� %s\n", filename);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* data = (char*)malloc(length + 1);
    if (data == NULL) {
        fclose(file);
        printf("�ڴ����ʧ��\n");
        return NULL;
    }
    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);
    return data;
}
double get_frame_rate(const char* video_file) {
    AVFormatContext* format_ctx = NULL;
    AVStream* video_stream = NULL;
    double frame_rate = 0.0;

    if (avformat_open_input(&format_ctx, video_file, NULL, NULL) != 0) {
        fprintf(stderr, "�޷�����Ƶ�ļ���\n");
        return 0.0;
    }

    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        fprintf(stderr, "�޷���ȡ��Ƶ�ļ�������Ϣ��\n");
        avformat_close_input(&format_ctx);
        return 0.0;
    }

    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream = format_ctx->streams[i];
            break;
        }
    }

    if (video_stream) {
        AVRational frame_rate_rational = av_guess_frame_rate(format_ctx, video_stream, NULL);
        frame_rate = av_q2d(frame_rate_rational);
    }

    avformat_close_input(&format_ctx);
    return frame_rate;
}

int merge_audio_video(const char* audio_file, const char* video_file, const char* output_file) {
    AVFormatContext* input_format_ctx_audio = NULL, * input_format_ctx_video = NULL, * output_format_ctx = NULL;
    AVOutputFormat* output_format = NULL;
    AVStream* audio_stream = NULL, * video_stream = NULL, * out_audio_stream = NULL, * out_video_stream = NULL;
    AVPacket packet;
    int ret;
    double frame_rate;

    // ��ȡ��Ƶ֡����
    frame_rate = get_frame_rate(video_file);
    if (frame_rate == 0.0) {
        fprintf(stderr, "�޷���ȡ��Ƶ֡���ʡ�\n");
        return -1;
    }

    // �������ļ�
    if ((ret = avformat_open_input(&input_format_ctx_audio, audio_file, 0, 0)) < 0) {
        fprintf(stderr, "�޷���������Ƶ�ļ���\n");
        return ret;
    }
    if ((ret = avformat_open_input(&input_format_ctx_video, video_file, 0, 0)) < 0) {
        fprintf(stderr, "�޷���������Ƶ�ļ���\n");
        return ret;
    }
    if ((ret = avformat_find_stream_info(input_format_ctx_audio, 0)) < 0) {
        fprintf(stderr, "�޷���ȡ��Ƶ�ļ�������Ϣ��\n");
        return ret;
    }
    if ((ret = avformat_find_stream_info(input_format_ctx_video, 0)) < 0) {
        fprintf(stderr, "�޷���ȡ��Ƶ�ļ�������Ϣ��\n");
        return ret;
    }
    avformat_alloc_output_context2(&output_format_ctx, NULL, NULL, output_file);
    if (!output_format_ctx) {
        fprintf(stderr, "�޷�������������ġ�\n");
        return AVERROR_UNKNOWN;
    }
    output_format = output_format_ctx->oformat;

    // �����Ƶ��
    audio_stream = input_format_ctx_audio->streams[0];
    out_audio_stream = avformat_new_stream(output_format_ctx, NULL);
    if (!out_audio_stream) {
        fprintf(stderr, "�޷����������Ƶ����\n");
        return AVERROR_UNKNOWN;
    }
    if ((ret = avcodec_parameters_copy(out_audio_stream->codecpar, audio_stream->codecpar)) < 0) {
        fprintf(stderr, "�޷�������Ƶ����������\n");
        return ret;
    }
    out_audio_stream->time_base = audio_stream->time_base;

    // �����Ƶ��
    video_stream = input_format_ctx_video->streams[0];
    out_video_stream = avformat_new_stream(output_format_ctx, NULL);
    if (!out_video_stream) {
        fprintf(stderr, "�޷����������Ƶ����\n");
        return AVERROR_UNKNOWN;
    }
    if ((ret = avcodec_parameters_copy(out_video_stream->codecpar, video_stream->codecpar)) < 0) {
        fprintf(stderr, "�޷�������Ƶ����������\n");
        return ret;
    }
    out_video_stream->time_base = (AVRational){ 1, (int)frame_rate };
    out_video_stream->codecpar->codec_tag = 0;

    // ������ļ�
    if (!(output_format->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&output_format_ctx->pb, output_file, AVIO_FLAG_WRITE)) < 0) {
            fprintf(stderr, "�޷�������ļ���\n");
            return ret;
        }
    }

    if ((ret = avformat_write_header(output_format_ctx, NULL)) < 0) {
        fprintf(stderr, "������ļ�ʱ��������\n");
        return ret;
    }

    // д����Ƶ���ݰ�
    while (av_read_frame(input_format_ctx_audio, &packet) >= 0) {
        packet.stream_index = out_audio_stream->index;
        av_packet_rescale_ts(&packet, audio_stream->time_base, out_audio_stream->time_base);
        av_interleaved_write_frame(output_format_ctx, &packet);
        av_packet_unref(&packet);
    }

    // д����Ƶ���ݰ�
    while (av_read_frame(input_format_ctx_video, &packet) >= 0) {
        packet.stream_index = out_video_stream->index;
        av_packet_rescale_ts(&packet, video_stream->time_base, out_video_stream->time_base);
        av_interleaved_write_frame(output_format_ctx, &packet);
        av_packet_unref(&packet);
    }

    av_write_trailer(output_format_ctx);

    avformat_close_input(&input_format_ctx_audio);
    avformat_close_input(&input_format_ctx_video);

    if (!(output_format->flags & AVFMT_NOFILE)) {
        avio_closep(&output_format_ctx->pb);
    }
    avformat_free_context(output_format_ctx);
    return 0;
}



void format_filename(char* filename) {
    char* src = filename, * dst = filename;
    while (*src) {
        // ֻ������ĸ�����֡��»��ߺ������ַ�
        if (isalnum((unsigned char)*src) || *src == '_' || (*src & 0x80)) {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

void processDirectory(const char* path);

void traverseDirectory(const char* basePath, DynamicArray* folders) {
    struct dirent* entry;
    struct stat statbuf;
    DIR* dp = opendir(basePath);
    if (dp == NULL) {
        perror("opendir");
        return;
    }
    while ((entry = readdir(dp))) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);
        if (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            addName(folders, path);
            printf("��ǰĿ¼: %s\n", path);

            // ����processDirectory����ÿ����Ŀ¼
            processDirectory(path);
        }
    }
    closedir(dp);
}

//void processDirectory(const char* path) {
//    struct dirent* entry;
//    struct stat statbuf;
//    DIR* dp = opendir(path);
//    if (dp == NULL) {
//        perror("opendir");
//        return;
//    }
//    while ((entry = readdir(dp))) {
//        char subPath[1024];
//        snprintf(subPath, sizeof(subPath), "%s/%s", path, entry->d_name);
//        if (stat(subPath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
//            char entryPath[1024];
//            snprintf(entryPath, sizeof(entryPath), "%s/entry.json", subPath);
//            printf("Processing directory: %s\n", subPath);
//
//            char* jsonContent = read_file(entryPath);
//            if (jsonContent) {
//                cJSON* root = cJSON_Parse(jsonContent);
//                if (root == NULL) {
//                    printf("����JSON�ļ�ʧ��: %s\n", entryPath);
//                }
//                else {
//                    cJSON* typeTag = cJSON_GetObjectItem(root, "type_tag");
//                    cJSON* title = cJSON_GetObjectItem(root, "title");
//                    if (typeTag != NULL && cJSON_IsString(typeTag) && title != NULL && cJSON_IsString(title)) {
//                        printf("type_tag: %s\n", typeTag->valuestring);
//                        printf("title: %s\n", title->valuestring);
//
//                        char formatted_title[256];
//                        strncpy_s(formatted_title, sizeof(formatted_title), title->valuestring, _TRUNCATE);
//                        format_filename(formatted_title);
//
//                        char targetDir[1024];
//                        snprintf(targetDir, sizeof(targetDir), "%s/%s", subPath, typeTag->valuestring);
//                        printf("Ŀ��Ŀ¼: %s\n", targetDir);
//
//                        char audioFile[1024];
//                        char videoFile[1024];
//                        snprintf(audioFile, sizeof(audioFile), "%s/audio.m4s", targetDir);
//                        snprintf(videoFile, sizeof(videoFile), "%s/video.m4s", targetDir);
//
//                        char outputFile[1024];
//                        snprintf(outputFile, sizeof(outputFile), "videotrans/%s.mp4", formatted_title);
//                        printf("����ļ�: %s\n", outputFile);
//                        if (merge_audio_video(audioFile, videoFile, outputFile) == 0) {
//                            printf("�ϲ����: %s\n", outputFile);
//                        }
//                        else {
//                            printf("�ϲ�ʧ��: %s\n", outputFile);
//                        }
//                    }
//                    else {
//                        printf("δ�ҵ�type_tag��title��ǩ: %s\n", entryPath);
//                    }
//                    cJSON_Delete(root);
//                }
//                free(jsonContent);
//            }
//            else {
//                printf("�޷���ȡ�ļ�: %s\n", entryPath);
//            }
//        }
//    }
//    closedir(dp);
//}


void processDirectory(const char* path) {
    struct dirent* entry;
    struct stat statbuf;
    DIR* dp = opendir(path);
    if (dp == NULL) {
        perror("opendir");
        return;
    }
    while ((entry = readdir(dp))) {
        char subPath[1024];
        snprintf(subPath, sizeof(subPath), "%s/%s", path, entry->d_name);
        if (stat(subPath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char entryPath[1024];
            snprintf(entryPath, sizeof(entryPath), "%s/entry.json", subPath);

            char* jsonContent = read_file(entryPath);
            if (jsonContent) {
                cJSON* root = cJSON_Parse(jsonContent);
                if (root == NULL) {
                    printf("����JSON�ļ�ʧ��\n");
                }
                else {
                    cJSON* typeTag = cJSON_GetObjectItem(root, "type_tag");
                    cJSON* title = cJSON_GetObjectItem(root, "title");
                    if (typeTag != NULL && cJSON_IsString(typeTag) && title != NULL && cJSON_IsString(title)) {
                        printf("type_tag: %s\n", typeTag->valuestring);
                        printf("title: %s\n", title->valuestring);

                        char formatted_title[256];
                        strncpy_s(formatted_title, sizeof(formatted_title), title->valuestring, _TRUNCATE);
                        format_filename(formatted_title);

                        char targetDir[1024];
                        snprintf(targetDir, sizeof(targetDir), "%s/%s", subPath, typeTag->valuestring);
                        printf("Ŀ��Ŀ¼: %s\n", targetDir);

                        char audioFile[1024];
                        char videoFile[1024];
                        snprintf(audioFile, sizeof(audioFile), "%s/audio.m4s", targetDir);
                        snprintf(videoFile, sizeof(videoFile), "%s/video.m4s", targetDir);

                        char outputFile[1024];
                        snprintf(outputFile, sizeof(outputFile), "videotrans/%s.mp4", formatted_title);
                        printf("����ļ�: %s\n", outputFile);
                        merge_audio_video(audioFile, videoFile, outputFile);
                        printf("�ϲ����: %s\n", outputFile);
                    }
                    else {
                        printf("δ�ҵ�type_tag��title��ǩ\n");
                    }
                    cJSON_Delete(root);
                }
                free(jsonContent);
            }
        }
    }
    closedir(dp);
}


int main() {
    setlocale(LC_ALL, "zh_CN.UTF-8");
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    DynamicArray* folders = createArray(INITIAL_SIZE);
    int vid_num = 0;
    char basePath[] = "bilibili_video";

    traverseDirectory(basePath, folders);
    vid_num = folders->size;

    printf("Number of folders: %d\n", vid_num);
    for (int i = 0; i < folders->size; i++) {
        printf("Folder %d: %s\n", i + 1, folders->names[i]);
    }

    freeArray(folders);
    return 0;
}




