#pragma once

#ifndef DATUM_H
#define DATUM_H
#include <cstdint>
#include <list>
#include <algorithm>
#include <limits>
#include <memory>

enum DatumLabel {
	DATUM_UNSPEC, DATUM_R, DATUM_G, DATUM_B, DATUM_A, DATUM_Y, DATUM_CB, DATUM_CR, DATUM_Z, DATUM_COMPOSITE, DATUM_A2, DATUM_Y2, DATUM_C,
	DATUM_UNSPEC2, DATUM_UNSPEC3, DATUM_UNSPEC4, DATUM_UNSPEC5, DATUM_UNSPEC6, DATUM_UNSPEC7, DATUM_UNSPEC8
};

enum DatumDType {
	DATUM_INT32, DATUM_DOUBLE
};

class DatumPlane {
public:
	DatumPlane(int w, int h, DatumDType dt, DatumLabel dl)
	{
		Allocate(w, h, dt, dl);
	}
	void Allocate(int w, int h, DatumDType dt, DatumLabel dl)
	{
		int i;
		m_n_lines = h;
		m_width = w;
		m_datum_label = dl;
		m_datum_type = dt;

		if (dt == DATUM_INT32)
			i_datum.resize(h);
		else
			lf_datum.resize(h);
		for (i = 0; i < m_n_lines; ++i)
		{
			if (dt == DATUM_INT32)
				i_datum[i].resize(m_width);
			else
				lf_datum[i].resize(m_width);
		}
		m_isallocated = true;
	}
	DatumPlane()
	{
		m_isallocated = false;
	}
	~DatumPlane()
	{
		//Destroy();
	}
	void Destroy(void)
	{
		int i;
		if (!m_isallocated)
			return;
		for (i=0; i< m_n_lines; ++i)
		{
			if (m_datum_type == DATUM_INT32)
				i_datum[i].clear();
			else
				lf_datum[i].clear();
		}
		if (m_datum_type == DATUM_INT32)
			i_datum.clear();
		else
			lf_datum.clear();
	}
	bool operator==(DatumLabel const &l) { return m_datum_label == l; } // used for find function
	/*DatumPlane& operator=(DatumPlane &dp)
	{
		if (this == &dp)
			return *this;

		m_isallocated = dp.m_isallocated;
		m_datum_label = dp.m_datum_label;
		m_n_lines = dp.m_n_lines;
		m_datum_type = dp.m_datum_type;
		m_width = dp.m_width;

		if (m_isallocated)
		{
			for (int i = 0; i < m_n_lines; ++i)
				std::memcpy(m_datum_line[i].i_datum, dp.m_datum_line[i].i_datum, m_width * ((m_datum_type == DATUM_INT32) ? sizeof(int32_t) : sizeof(double)));
		}

		return *this;
	} */
	DatumLabel m_datum_label;
	DatumDType m_datum_type;
	int m_n_lines;
	std::vector<std::vector<int32_t>> i_datum;
	std::vector<std::vector<double>> lf_datum;
	int m_width;
	bool m_isallocated = false;
};


/*
class DatumImageElement
{
public:
	DatumImageElement() { ;  }
	DatumImageElement(int w, int h, int bpc)
	{
		m_width = w;
		m_height = h;
		m_nbits = bpc;
		m_is_initialized = 1;
	}
	int32_t GetPixelI(int x, int y, DatumLabel dtype)
	{
		std::list<class DatumPlane>::iterator it = std::find(planes.begin(), planes.end(), dtype);
		if (it == planes.end())
			return(INT32_MAX);
		else
			return(*it.datum_line[y].i_datum[x]);
	}
	double GetPixelLf(int x, int y, DatumLabel dtype)
	{
		std::list<class DatumPlane>::iterator it = std::find(planes.begin(), planes.end(), dtype);
		if (it == planes.end())
			return(std::numeric_limits<double>::max());
		else
			return(*it.datum_line[y].lf_datum[x]);
	}
	void SetPixelUi(int x, int y, DatumLabel dtype, uint32_t d)
	{
		std::list<class DatumPlane>::iterator it = std::find(planes.begin(), planes.end(), dtype);
		if (it != planes.end())
			*it.datum_line[y].ui_datum[x] = d;
	}
	void SetPixelI(int x, int y, DatumLabel dtype, int32_t d)
	{
		std::list<class DatumPlane>::iterator it = std::find(planes.begin(), planes.end(), dtype);
		if (it != planes.end())
			*it.datum_line[y].i_datum[x] = d;
	}
	void SetPixelLf(int x, int y, DatumLabel dtype, double d)
	{
		std::list<class DatumPlane>::iterator it = std::find(planes.begin(), planes.end(), dtype);
		if (it != planes.end())
			*it.datum_line[y].lf_datum[x] = d;
	}
	void AddPlane(DatumLabel dtype)
	{
		DatumPlane *dp;

		dp = new DatumPlane(width, height, dtype);
		planes.push_back(dp);
	}
	int  IsInitialized(void) { return m_is_initialized; }

// Member variables:
	int m_is_initialized = 0;
	int m_width;
	int m_height;
	int m_nbits;
	int m_is_compressed;
	int *m_n_datum_values_per_line;  // Number of datum vales on each line (all lines same for uncompressed/varies per line for compressed)
	std::vector<class DatumPlane> planes;
};

*/

#endif

