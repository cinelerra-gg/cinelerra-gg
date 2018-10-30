
/*
 * CINELERRA
 * Copyright (C) 2014 Adam Williams <broadcast at earthling dot net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "asset.h"
#include "bcsignals.h"
#include "file.h"
#include "filegif.h"
#include "gif_lib.h"
#include "mainerror.h"
#include "interlacemodes.h"
#include "vframe.h"
#include <string.h>


static int gif_err = 0;
#define GIF_ERR ,&gif_err
#define GifErrorString(s) Gif##ErrorString(gif_err)
#define GifLastError(s) gif_err

const char *gifErrorString()
{
	static char msg[32];
	snprintf(msg, sizeof(msg), "Gif Error %d", GifLastError());
	return msg;
}

FileGIF::FileGIF(Asset *asset, File *file)
 : FileList(asset, file, "GIFLIST", ".gif", FILE_GIF, FILE_GIF_LIST)
{
}

FileGIF::~FileGIF()
{
}

int FileGIF::check_sig(Asset *asset)
{
	FILE *stream = fopen(asset->path, "rb");

	if(stream)
	{
		char test[8];
		int ret = fread(test, 1, 6, stream);
		fclose(stream);

		if( ret >= 6 &&
			test[0] == 'G' && test[1] == 'I' && test[2] == 'F' &&
			test[3] == '8' && test[4] == '7' && test[5] == 'A')
		{
			eprintf("FileGIFF: version error (87A): \"%s\".\n", asset->path);
			return 1;
		}
	}

	if(strlen(asset->path) > 4)
	{
		int len = strlen(asset->path);
		if(!strncasecmp(asset->path + len - 4, ".gif", 4)) return 1;
	}
	return 0;
}

int FileGIF::colormodel_supported(int colormodel)
{
	return BC_RGB888;
}

int FileGIF::get_best_colormodel(Asset *asset, int driver)
{
	return BC_RGB888;
}


int FileGIF::read_frame_header(char *path)
{
	FILE *stream = fopen(path, "rb");

	if(stream)
	{
		unsigned char test[16];
		int ret = fread(test, 16, 1, stream);
		fclose(stream);
		if( ret < 1 ) return 1;
		asset->width = test[6] | (test[7] << 8);
		asset->height = test[8] | (test[9] << 8);
//printf("FileGIF::read_frame_header %d %d %d\n", __LINE__, asset->width, asset->height);
		return 0;
	}

	perror(path);
	return 1;
}


static int input_func(GifFileType *gif_file, GifByteType *buffer, int bytes)
{
	FileGIF *file = (FileGIF*)gif_file->UserData;
	if(file->offset + bytes > file->size) bytes = file->size - file->offset;
	if(bytes > 0)
	{
		memcpy(buffer, file->data + file->offset, bytes);
		file->offset += bytes;
	}
	return bytes;
}

int FileGIF::read_frame(VFrame *output, VFrame *input)
{
	data = input->get_data();
	offset = 0;
	size = input->get_compressed_size();

	GifFileType *gif_file;
	GifRowType *gif_buffer;
	gif_file = DGifOpen(this, input_func GIF_ERR);


	if(gif_file == 0)
	{
		eprintf("FileGIF::read_frame %d: %s\n", __LINE__, GifErrorString());
		return 1;
	}
	gif_buffer = (GifRowType*)malloc(sizeof(GifRowType) * gif_file->SHeight);
	int row_size = gif_file->SWidth * sizeof(GifPixelType);
	gif_buffer[0] = (GifRowType)malloc(row_size);

	for(int i = 0; i < gif_file->SWidth; i++)
	{
		gif_buffer[0][i] = gif_file->SBackGroundColor;
	}

	for(int i = 0; i < gif_file->SHeight; i++)
	{
		gif_buffer[i] = (GifRowType)malloc(row_size);
		memcpy(gif_buffer[i], gif_buffer[0], row_size);
	}

	GifRecordType record_type;
	do
	{
		if(DGifGetRecordType(gif_file, &record_type) == GIF_ERROR)
		{
			eprintf("FileGIF::read_frame %d: %s\n", __LINE__, GifErrorString());
			break;
		}

		switch(record_type)
		{
			case IMAGE_DESC_RECORD_TYPE:
			{
				if(DGifGetImageDesc(gif_file) == GIF_ERROR)
				{
					eprintf("FileGIF::read_frame %d: %s\n", __LINE__, GifErrorString());
					break;
				}

				int row = gif_file->Image.Top;
				int col = gif_file->Image.Left;
				int width = gif_file->Image.Width;
				int height = gif_file->Image.Height;
				if(gif_file->Image.Left + gif_file->Image.Width > gif_file->SWidth ||
		 		  gif_file->Image.Top + gif_file->Image.Height > gif_file->SHeight)
				{
					DGifCloseFile(gif_file GIF_ERR);
					for(int k = 0; k < gif_file->SHeight; k++)
					{
						free(gif_buffer[k]);
					}
					free(gif_buffer);
					return 1;
				}

				if (gif_file->Image.Interlace)
				{
				    static int InterlacedOffset[] = { 0, 4, 2, 1 };
				    static int InterlacedJumps[] = { 8, 8, 4, 2 };
/* Need to perform 4 passes on the images: */
		    		for (int i = 0; i < 4; i++)
					{
						for (int j = row + InterlacedOffset[i];
							j < row + height;
							j += InterlacedJumps[i])
						{
			    			if (DGifGetLine(gif_file,
								&gif_buffer[j][col],
								width) == GIF_ERROR)
							{
								DGifCloseFile(gif_file GIF_ERR);
								for(int k = 0; k < gif_file->SHeight; k++)
								{
									free(gif_buffer[k]);
								}
								free(gif_buffer);
								return 1;
			    			}
						}
					}
				}
				else
				{
		    		for (int i = 0; i < height; i++)
					{
						if (DGifGetLine(gif_file, &gif_buffer[row++][col],
							width) == GIF_ERROR)
						{
							DGifCloseFile(gif_file GIF_ERR);
							for(int k = 0; k < gif_file->SHeight; k++)
							{
								free(gif_buffer[k]);
							}
							free(gif_buffer);
			    			return 1;
						}
		    		}
				}

				break;
			}
			default:
				break;
		}

	} while(record_type != TERMINATE_RECORD_TYPE);


	//int background = gif_file->SBackGroundColor;
	ColorMapObject *color_map = (gif_file->Image.ColorMap
		? gif_file->Image.ColorMap
		: gif_file->SColorMap);
	if(!color_map)
	{
		DGifCloseFile(gif_file GIF_ERR);
		for(int k = 0; k < gif_file->SHeight; k++)
		{
			free(gif_buffer[k]);
		}
		free(gif_buffer);
		return 1;
	}

	int screen_width = gif_file->SWidth;
	int screen_height = gif_file->SHeight;
	for(int i = 0; i < screen_height; i++)
	{
		GifRowType gif_row = gif_buffer[i];
		unsigned char *out_ptr = output->get_rows()[i];
		for(int j = 0; j < screen_width; j++)
		{
			GifColorType *color_map_entry = &color_map->Colors[gif_row[j]];
			*out_ptr++ = color_map_entry->Red;
			*out_ptr++ = color_map_entry->Green;
			*out_ptr++ = color_map_entry->Blue;
		}
	}


	for(int k = 0; k < gif_file->SHeight; k++)
	{
		free(gif_buffer[k]);
	}
	free(gif_buffer);
	DGifCloseFile(gif_file GIF_ERR);
	return 0;
}

