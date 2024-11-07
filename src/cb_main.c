/**
 * @brief main file
 */

#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>

#include "cb.h"

bool startsWith( const char *str, const char *start ) {
    return strncmp(str, start, strlen(start)) == 0;
}

bool utf8StrNextChar( const char **str, uint32_t *charDst ) {
    *charDst = 0;

    int8_t *dst = (int8_t *)charDst;

    dst[0] = *(*str)++;

    if (dst[0] == '\0')
        return true;

    // 1 char
    if ((dst[0] & 0x80) == 0x00)
        return true;

    // 2 chars
    if ((dst[0] & 0xE0) == 0xC0) {
        dst[1] = *(*str)++;

        return true
            && (dst[1] & 0xC0) == 0x80;
    }

    // 3 chars
    if ((dst[0] & 0xF0) == 0xE0) {
        dst[1] = *(*str)++;
        dst[2] = *(*str)++;
        return true
            && (dst[1] & 0xC0) == 0x80
            && (dst[2] & 0xC0) == 0x80
        ;
    }

    // 4 chars
    if ((dst[0] & 0xF1) == 0xF0) {
        dst[1] = *(*str)++;
        dst[2] = *(*str)++;
        dst[3] = *(*str)++;
        return true
            && (dst[1] & 0xC0) == 0x80
            && (dst[2] & 0xC0) == 0x80
            && (dst[3] & 0xC0) == 0x80
        ;
    }

    return false;
}

bool utf8StrLength( const char *str, size_t *dst ) {
    uint32_t ch = 0;
    size_t len = 0;

    for (;;) {
        if (!utf8StrNextChar(&str, &ch))
            return false;
        if (ch == 0) {
            *dst = len;
            return true;
        }
        len++;
    }
}

int main( void ) {
    Cb cb = cbCtor("пустота");

    setlocale(LC_ALL, "RU");

    while (true) {
        char commandBuffer[512] = {0};

        printf(">>> ");
        if (fgets(commandBuffer, sizeof(commandBuffer), stdin) == NULL)
            continue;

        if (startsWith(commandBuffer, "выход"))
            break;

        if (startsWith(commandBuffer, "вывести")) {
            cbDump(stdout, cb);
            continue;
        }

        if (startsWith(commandBuffer, "сохранить")) {
            char buffer[512] = {0};
            bool isDot = false;

            printf("    Формат? ([c]b, [d]ot) ");
            fgets(buffer, sizeof(buffer), stdin);

            if (buffer[0] == 'c') {
                isDot = false;
            } else if (buffer[0] == 'd') {
                isDot = true;
            } else {
                const size_t len = strlen(buffer);
                if (len > 0)
                    buffer[len - 1] = '\0';

                printf("    неизвестный формат: %s", buffer);
                continue;
            }


            printf("    Путь? ");
            fgets(buffer, sizeof(buffer), stdin);

            const size_t len = strlen(buffer);
            if (len > 0)
                buffer[len - 1] = '\0';

            FILE *file = fopen(buffer, "w");
            if (file == NULL) {
                printf("    Ошибка открытия файла: %s\n", strerror(errno));
                continue;
            }

            if (isDot) {
                cbDumpDot(file, cb);
            } else {
                cbDump(file, cb);
            }
            fclose(file);

            continue;
        }

        if (startsWith(commandBuffer, "загрузить")) {
            char buffer[512] = {0};

            printf("    Путь? ");
            fgets(buffer, sizeof(buffer), stdin);

            const size_t len = strlen(buffer);
            if (len > 0)
                buffer[len - 1] = '\0';

            FILE *file = fopen(buffer, "r");
            if (file == NULL) {
                printf("    Ошибка открытия файла: %s\n", strerror(errno));
                continue;
            }


            fseek(file, 0, SEEK_END);
            size_t size = ftell(file);
            fseek(file, 0, SEEK_SET);

            char *text = (char *)calloc(size, sizeof(char));

            if (text == NULL) {
                fclose(file);
                printf("    Ошибка выделения памяти.\n");
                continue;
            }

            fread(text, 1, size, file);

            fclose(file);

            Cb newCb = NULL;

            if (!cbParse(text, &newCb)) {
                printf("    Ошибка парсинга");
                continue;
            }

            cbDtor(cb);
            cb = newCb;

            continue;
        }

        if (startsWith(commandBuffer, "начать")) {
            // start session
            CbIter iter = cbIter(cb);
            char sessionBuffer[512] = {0};

            while (!cbIterFinished(&iter)) {
                const char *text = cbIterGetText(&iter);

                printf("Он/она/оно %s? [Д]а/[Н]ет ", text);

                fgets(sessionBuffer, sizeof(sessionBuffer), stdin);

                cbIterNext(&iter, !startsWith(sessionBuffer, "Н") && !startsWith(sessionBuffer, "н"));
            }

            printf("Это %s? [Д]а/[Н]ет ", cbIterGetText(&iter));

            fgets(sessionBuffer, sizeof(sessionBuffer), stdin);

            if (startsWith(sessionBuffer, "Н") || startsWith(sessionBuffer, "н")) {
                char conditionBuffer[512] = {0};

                printf("Тогда это ...? ");
                fgets(sessionBuffer, sizeof(sessionBuffer), stdin);
                printf("Потому что он/она/оно ...? ");
                fgets(conditionBuffer, sizeof(conditionBuffer), stdin);

                sessionBuffer[strlen(sessionBuffer) - 1] = '\0';
                conditionBuffer[strlen(conditionBuffer) - 1] = '\0';

                if (!cbIterInsertCorrect(&iter, conditionBuffer, sessionBuffer)) {
                    printf("Произошла внутренняя ошибка...\n");
                    break;
                }
            }


            continue;
        }

        printf("    неизвестная комманда: %s\n", commandBuffer);
    }

    cbDtor(cb);

    return 0;
} // main

// cb_main.c
