#pragma once

#include <fstream>
#include <filesystem>

#include "NBS_Reader.hpp"
#include "NBS_Writer.hpp"

class NBS_IO
{
	NBS_IO(void) = delete;
	~NBS_IO(void) = delete;

public:
	class InputStream
	{
	private:
		std::fstream &refStream;

	public:
		InputStream(const InputStream &) = delete;
		InputStream(InputStream &&) noexcept = delete;
		InputStream &operator=(const InputStream &) = delete;
		InputStream &operator=(InputStream &&) noexcept = delete;
		
		InputStream(std::fstream &stream) : refStream(stream)
		{}
		~InputStream(void) = default;

		bool HasValidData(size_t szSize)
		{
			return true;
		}

		bool GetRange(void *pData, size_t szSize)
		{
			return !refStream.read((char *)pData, szSize).fail();
		}
	};
	static_assert(NBS_Reader_Helper::InputStreamLike<InputStream>);

	class OutputStream
	{
	private:
		std::fstream &refStream;

	public:
		OutputStream(const OutputStream &) = delete;
		OutputStream(OutputStream &&) noexcept = delete;
		OutputStream &operator=(const OutputStream &) = delete;
		OutputStream &operator=(OutputStream &&) noexcept = delete;

		OutputStream(std::fstream &stream) : refStream(stream)
		{}
		~OutputStream(void) = default;

		bool AddReserve(size_t szAddSize)
		{
			return true;
		}

		bool PutRange(const void *pData, size_t szSize)
		{
			return !refStream.write((const char *)pData, szSize).fail();
		}
	};
	static_assert(NBS_Writer_Helper::OutputStreamLike<OutputStream>);

public:
	static bool ReadNBSFromFile(NBS_File &fileNBS, const std::filesystem::path &path)
	{
		std::fstream fRead(path, std::ios_base::in | std::ios_base::binary);
		if (!fRead.is_open())
		{
			return false;
		}

		NBS_IO::InputStream fIptStream(fRead);
		return NBS_Reader::ReadNBS(fileNBS, fIptStream);
	}

	static bool WriteNBSToFile(const NBS_File &fileNBS, const std::filesystem::path &path)
	{
		std::fstream fWrite(path, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
		if (!fWrite.is_open())
		{
			return false;
		}

		NBS_IO::OutputStream fOptStream(fWrite);
		return NBS_Writer::WriteNBS(fileNBS, fOptStream);
	}
};
