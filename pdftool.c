#include <pdftool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <zlib.h>

#define oldchar 15

static float extract_number(const char* search, int lastcharoffset)
{
	int i = lastcharoffset;
	while (i > 0 && search[i] == ' ') i--;
	while (i > 0 && (isdigit(search[i]) || search[i] == '.')) i--;
	float flt =- 1.0f;
	char buffer[oldchar + 5]; 
	memset(buffer, 0, sizeof(buffer));
	strncpy(buffer, search + i + 1, lastcharoffset - i);
	if (buffer[0] && sscanf(buffer, "%f", &flt)) return flt;
	return -1.0f;
}

static bool seen2(const char* search, char* recent)
{
	if (    recent[oldchar - 3] == search[0] 
		&& recent[oldchar - 2] == search[1] 
		&& (recent[oldchar - 1] == ' ' || recent[oldchar - 1] == 0x0d || recent[oldchar - 1] == 0x0a) 
		&& (recent[oldchar - 4] == ' ' || recent[oldchar - 4] == 0x0d || recent[oldchar - 4] == 0x0a)
		) { return true; }
	return false;
}

static void process_pdf_output(FILE* file, char* output, size_t len)
{
	bool intextobject = false;
	bool nextliteral = false;
	int rbdepth = 0, j = 0;

	char oc[oldchar];
	for (j = 0; j < oldchar; j++) {
		oc[j] = ' ';
	}

	for (size_t i = 0; i < len; i++) {
		char c = output[i];
		//printf("%c", c);
		if (intextobject) {
			if (rbdepth == 0 && seen2("TD", oc)) {
				float num = extract_number(oc, oldchar - 5);
				if (num > 1.0) {
					fputc(0x0d, file);
					fputc(0x0a, file);
				}
				if (num < 1.0f)fputc('\t', file);
			}
			if (rbdepth == 0 && seen2("ET", oc)) {
				intextobject = false;
				fputc(0x0d, file);
				fputc(0x0a, file);
			}
			else if (c == '(' && rbdepth == 0 && !nextliteral) {
				rbdepth = 1;
				int num = extract_number(oc,oldchar-1);
				if (num > 0) {
					if (num > 1000.0) fputc('\t', file);
					else if (num > 100.0) fputc(' ', file);
				}
			}
			else if (c == ')' && rbdepth == 1 && !nextliteral) {
				rbdepth = 0;
			}
			else if (rbdepth == 1) {
				if (c == '\\' && !nextliteral) {
					nextliteral = true;
				} else {
					nextliteral = false;
					fputc(c, file);
				}
			}
		}
		for (j = 0; j < oldchar - 1; j++) {
			oc[j] = oc[j+1];
		}
		oc[oldchar - 1] = c;
		if (!intextobject) {
			if (seen2("BT", oc)) {
				intextobject = true;
			}
		}
	}
}

size_t sub_string_find(char* buffer, const char* search, size_t buffersize)
{
	char* buffer0 = buffer;

	size_t len = strlen(search);
	bool fnd = false;
	while (!fnd) {
		fnd = true;
		for (size_t i = 0; i < len; i++) {
			if (buffer[i] != search[i]) {
				fnd = false;
				break;
			}
		}
		if (fnd) return buffer - buffer0;
		buffer = buffer + 1;
		if (buffer - buffer0 + len >= buffersize) return -1;
	}
	return -1;
}

void pdf_to_txt_file(const char* in_pdf, const char* out_txt)
{
	FILE* fileo = fopen(out_txt, "w");
	if (fileo) fclose(fileo);
	fileo = fopen(out_txt, "a");

	FILE* filei = fopen(in_pdf, "rb");

	if (filei && fileo) {
		int fseekres = fseek(filei, 0, SEEK_END);  
		long filelen = ftell(filei);
		fseekres = fseek(filei, 0, SEEK_SET);

		char* buffer = (char*)malloc(filelen);
		fread(buffer, filelen, 1 ,filei);

		bool morestreams = true;

		while (morestreams) {
			size_t streamstart = sub_string_find(buffer, "stream", filelen);
			size_t streamend   = sub_string_find(buffer, "endstream", filelen);
			if (streamstart > 0 && streamend>streamstart) {
				streamstart += 6;

				if (buffer[streamstart] == 0x0d && buffer[streamstart+1] == 0x0a) streamstart += 2;
				else if (buffer[streamstart] == 0x0a) streamstart++;

				if (buffer[streamend-2] == 0x0d && buffer[streamend-1] == 0x0a) streamend -= 2;
				else if (buffer[streamend-1] == 0x0a) streamend--;

				size_t outsize = (streamend - streamstart) * 10;
				char* output = (char*)malloc(outsize); 
				memset(output, 0, outsize);

				z_stream zstrm; 
				memset(&zstrm, 0, sizeof(zstrm));

				zstrm.avail_in = streamend - streamstart + 1;
				zstrm.avail_out = outsize;
				zstrm.next_in = (Bytef*)(buffer + streamstart);
				zstrm.next_out = (Bytef*)output;

				int rsti = inflateInit(&zstrm);
				if (rsti == Z_OK) {
					int rst2 = inflate (&zstrm, Z_FINISH);
					if (rst2 >= 0) {
						size_t totout = zstrm.total_out;
						process_pdf_output(fileo, output, totout);
					}
				}
				free(output); 
				output = 0;
				buffer += streamend + 7;
				filelen = filelen - (streamend + 7);
			} else morestreams = false;
		}
		fclose(filei);
	}
	if (fileo) fclose(fileo);
}

char* file_string_get(const char* path, long* file_size)
{
    FILE* file = fopen(path, "rb");
    if (!file) {
        printf("Could not open file '%s'\n", path);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    char* buffer = (char*)malloc(size);
    fseek(file, 0, SEEK_SET);
    fread(buffer, 1, size, file);
    fclose(file);
    *file_size = size;
    return buffer;
}

char* pdf_string_get(const char* path, long* size)
{
	char out[16] = "temp.txt";
	pdf_to_txt_file(path, out);
    char* file_buffer = file_string_get(out, size);
	system("rm temp.txt");
	return file_buffer;
}
