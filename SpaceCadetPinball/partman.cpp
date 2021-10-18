#include "pch.h"
#include "partman.h"

#include "gdrv.h"
#include "GroupData.h"
#include "utils.h"
#include "zdrv.h"

short partman::_field_size[] =
{
	2, -1, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0
};

DatFile* partman::load_records(LPCSTR lpFileName, bool fullTiltMode)
{
	datFileHeader header{};
	dat8BitBmpHeader bmpHeader{};
	dat16BitBmpHeader zMapHeader{};

	auto fileHandle = fopen(lpFileName, "rb");
	if (fileHandle == nullptr)
		return nullptr;

	fread(&header, 1, sizeof header, fileHandle);

	header.FileSize = utils::swap_i32(header.FileSize);
	header.NumberOfGroups = utils::swap_u16(header.NumberOfGroups);
	header.SizeOfBody = utils::swap_i32(header.SizeOfBody);
	header.Unknown = utils::swap_u16(header.Unknown);

	if (strcmp("PARTOUT(4.0)RESOURCE", header.FileSignature) != 0)
	{
		fclose(fileHandle);
		return nullptr;
	}

	auto datFile = new DatFile();
	if (!datFile)
	{
		fclose(fileHandle);
		return nullptr;
	}

	datFile->AppName = header.AppName;
	datFile->Description = header.Description;

	if (header.Unknown)
	{
		auto unknownBuf = new char[header.Unknown];
		if (!unknownBuf)
		{
			fclose(fileHandle);
			delete datFile;
			return nullptr;
		}
		fread(unknownBuf, 1, header.Unknown, fileHandle);
		delete[] unknownBuf;
	}

	datFile->Groups.reserve(header.NumberOfGroups);
	bool abort = false;
	for (auto groupIndex = 0; !abort && groupIndex < header.NumberOfGroups; ++groupIndex)
	{
		auto entryCount = LRead<uint8_t>(fileHandle);
		auto groupData = new GroupData(groupIndex);
		groupData->ReserveEntries(entryCount);

		for (auto entryIndex = 0; entryIndex < entryCount; ++entryIndex)
		{
			auto entryData = new EntryData();
			auto entryType = static_cast<FieldTypes>(LRead<uint8_t>(fileHandle));
			entryData->EntryType = entryType;

			int fixedSize = _field_size[static_cast<int>(entryType)];
			size_t fieldSize = fixedSize >= 0 ? fixedSize : utils::swap_u32(LRead<uint32_t>(fileHandle));
			entryData->FieldSize = static_cast<int>(fieldSize);

			if (entryType == FieldTypes::Bitmap8bit)
			{
				fread(&bmpHeader, 1, sizeof(dat8BitBmpHeader), fileHandle);
				
				bmpHeader.Width = utils::swap_i16(bmpHeader.Width);
				bmpHeader.Height = utils::swap_i16(bmpHeader.Height);
				bmpHeader.XPosition = utils::swap_i16(bmpHeader.XPosition);
				bmpHeader.YPosition = utils::swap_i16(bmpHeader.YPosition);
				bmpHeader.Size = utils::swap_i32(bmpHeader.Size);
				assertm(bmpHeader.Size + sizeof(dat8BitBmpHeader) == fieldSize, "partman: Wrong bitmap field size");
				assertm(bmpHeader.Resolution <= 2, "partman: bitmap resolution out of bounds");

				auto bmp = new gdrv_bitmap8(bmpHeader);
				entryData->Buffer = reinterpret_cast<char*>(bmp);
				fread(bmp->IndexedBmpPtr, 1, bmpHeader.Size, fileHandle);
			}
			else if (entryType == FieldTypes::Bitmap16bit)
			{
				/*Full tilt has extra byte(@0:resolution) in zMap*/
				auto zMapResolution = 0u;
				if (fullTiltMode)
				{
					zMapResolution = LRead<uint8_t>(fileHandle);
					fieldSize--;
					assertm(zMapResolution <= 2, "partman: zMap resolution out of bounds");
				}

				fread(&zMapHeader, 1, sizeof(dat16BitBmpHeader), fileHandle);
				zMapHeader.Width = utils::swap_i16(zMapHeader.Width);
				zMapHeader.Height = utils::swap_i16(zMapHeader.Height);
				zMapHeader.Stride = utils::swap_i16(zMapHeader.Stride);
				zMapHeader.Unknown0 = utils::swap_i32(zMapHeader.Unknown0);
				zMapHeader.Unknown1_0 = utils::swap_i16(zMapHeader.Unknown1_0);
				zMapHeader.Unknown1_1 = utils::swap_i16(zMapHeader.Unknown1_1);

				auto length = fieldSize - sizeof(dat16BitBmpHeader);

				auto zMap = new zmap_header_type(zMapHeader.Width, zMapHeader.Height, zMapHeader.Stride);
				zMap->Resolution = zMapResolution;
				if (zMapHeader.Stride * zMapHeader.Height * 2u == length)
				{
					fread(zMap->ZPtr1, 1, length, fileHandle);
				}
				else
				{
					// 3DPB .dat has zeroed zMap headers, in groups 497 and 498, skip them.
					fseek(fileHandle, static_cast<int>(length), SEEK_CUR);
				}
				entryData->Buffer = reinterpret_cast<char*>(zMap);
			}
			else
			{
				auto entryBuffer = new char[fieldSize];
				entryData->Buffer = entryBuffer;
				if (!entryBuffer)
				{
					abort = true;
					break;
				}
				fread(entryBuffer, 1, fieldSize, fileHandle);

				if (entryType == FieldTypes::ShortValue)
				{
					char c1 = entryBuffer[0];
					char c2 = entryBuffer[1];
					entryBuffer[0] = c2;
					entryBuffer[1] = c1;
				}
				else if (entryType == FieldTypes::Unknown2)
				{
					// TODO: This is 2 bytes according to partman::_field_size. Should be endian swapped
				}
				else if (entryType == FieldTypes::ShortArray)
				{
					for (size_t i = 0; i < fieldSize; i += 2)
					{
						char c1 = entryBuffer[i + 0];
						char c2 = entryBuffer[i + 1];
						entryBuffer[i + 0] = c2;
						entryBuffer[i + 1] = c1;
					}
				}
				else if (entryType == FieldTypes::FloatArray)
				{
					for (size_t i = 0; i < fieldSize; i += 4)
					{
						char c1 = entryBuffer[i + 0];
						char c2 = entryBuffer[i + 1];
						char c3 = entryBuffer[i + 2];
						char c4 = entryBuffer[i + 3];
						entryBuffer[i + 0] = c4;
						entryBuffer[i + 1] = c3;
						entryBuffer[i + 2] = c2;
						entryBuffer[i + 3] = c1;
					}
				}
			}

			groupData->AddEntry(entryData);
		}

		datFile->Groups.push_back(groupData);
	}

	fclose(fileHandle);
	if (datFile->Groups.size() == header.NumberOfGroups)
	{
		datFile->Finalize();
		return datFile;
	}
	delete datFile;
	return nullptr;
}
