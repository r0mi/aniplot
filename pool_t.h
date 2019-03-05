// Elmo Trolla, 2019
// Licence: pick one - public domain / UNLICENCE (https://www.unlicense.org) / MIT (https://opensource.org/licenses/MIT).

#ifndef __POOL_T_H__
#define __POOL_T_H__

//
// This pool is a two-level memory store - an array of pointers to segments.
// Each segment is a dynamically allocated array of items of type T.
// m_segmentlist is the array of pointers to segments, and will be reallocated if more memory is needed.
//
//
//  pool_t<int> pool(3, 2);
//
//    append  |     what happens
//     item   |        inside
//  ------------------------------------------------------------------------------------------------------
//       .    |                           // allocate m_segmentlist with 2 entries.
//       .    |                           // allocate the first segment to m_segmentlist[0]
//      11    | m_segmentlist[0][0] = 11;
//      12    | m_segmentlist[0][1] = 12;
//      13    | m_segmentlist[0][2] = 13;
//       .    |                           // allocate the second segment and write ptr to m_segmentlist[1]
//      14    | m_segmentlist[1][0] = 14;
//      15    | m_segmentlist[1][1] = 15;
//      16    | m_segmentlist[1][2] = 16;
//       .    |                           // release m_segmentlist and reallocate twice the size.
//       .    |                           // copy pointers from old m_segmentlist to new list.
//       .    |                           // allocate the third segment and write ptr to m_segmentlist[2]
//      17    | m_segmentlist[2][2] = 17;
//      18    | m_segmentlist[2][2] = 18;
//

#include <string.h> // memset
#include <assert.h>

#include <stdio.h>


template<class T>
class pool_t
{
public:

	// If num elements appended goes over num_elements_in_segment*initial_num_segments, then
	// twice as many segments will be allocated and old data will be copied to the new array.
	pool_t(int num_elements_in_segment=100000, int initial_num_segments=10000);
	~pool_t();

	void clear();

	void append(T& item);
	// Append and return pointer to the appended object. Content of the returned object is undefined.
	T*   append();

	// -size..size-1. 0 is the first one appended. -1 is the last element.
	T*   get(int i);

	// append() and clear() will increment this. Will be zero after creation. Nothing can reset it.
	int  get_change_counter();
	void inc_change_counter();

	// Return num elements appended.
	int  size();


	// Execute some internal tests. Should probably remove..
	void test();


protected:

	int   m_size;
	int   m_change_counter;
	int   m_num_elements_in_segment;
	int   m_initial_num_segments;
	int   m_segments_capacity;

	T**   m_segmentlist;

	void  m_grow_segmentlist();
};


// --------------------------------------------------------------------------
// ---- LIFECYCLE -----------------------------------------------------------
// --------------------------------------------------------------------------


template<class T>
pool_t<T>::pool_t(int num_elements_in_segment, int initial_num_segments)
{
	m_size = 0;
	m_change_counter = 0;
	m_num_elements_in_segment = num_elements_in_segment;
	m_initial_num_segments = initial_num_segments;
	m_segments_capacity = 0;
	m_segmentlist = NULL;
}


template<class T>
pool_t<T>::~pool_t()
{
	clear();
}


// --------------------------------------------------------------------------
// ---- METHODS -------------------------------------------------------------
// --------------------------------------------------------------------------


template<class T>
void pool_t<T>::clear()
{
	if (m_segmentlist) {
		for (int i = 0; i < m_segments_capacity; i++) {
			delete[] (char*)m_segmentlist[i];
		}
		delete[] (char*)m_segmentlist;
		m_size = 0;
		m_segmentlist = NULL;
		m_segments_capacity = 0;
		m_change_counter++;
	}
}


template<class T>
void pool_t<T>::append(T& item)
{
	*(append()) = item;
}


template<class T>
T* pool_t<T>::append()
{
	int index_in_segment = m_size % m_num_elements_in_segment;
	int segment_index    = m_size / m_num_elements_in_segment;

	if (m_segments_capacity <= segment_index) {
		m_grow_segmentlist();
	}

	if (m_segmentlist[segment_index] == NULL) {
		m_segmentlist[segment_index] = (T*)new char[m_num_elements_in_segment * sizeof(T)];
	}

	m_size++;
	m_change_counter++;

	return &m_segmentlist[segment_index][index_in_segment];
}


template<class T>
T* pool_t<T>::get(int i)
{
	if (i >= m_size) return NULL;
	if (i < 0) i += m_size;
	if (i < 0) return NULL;

	int index_in_segment = i % m_num_elements_in_segment;
	int segment_index    = i / m_num_elements_in_segment;

	return &m_segmentlist[segment_index][index_in_segment];
}


template<class T> int  pool_t<T>::get_change_counter() { return m_change_counter; }
template<class T> void pool_t<T>::inc_change_counter() { m_change_counter++; }
template<class T> int  pool_t<T>::size()               { return m_size; }


// --------------------------------------------------------------------------
// ---- PRIVATE -------------------------------------------------------------
// --------------------------------------------------------------------------


template<class T>
void pool_t<T>::m_grow_segmentlist()
{
	T** old = m_segmentlist;

	if (old) {
		m_segmentlist = (T**)new char[m_segments_capacity * 2 * sizeof(T*)];
		memset(m_segmentlist + sizeof(T*) * m_segments_capacity, 0, sizeof(T*) * m_segments_capacity);
		memmove(m_segmentlist, old, sizeof(T*) * m_segments_capacity);
		m_segments_capacity = m_segments_capacity * 2;
		delete[] (char*)old;

	} else {
		m_segments_capacity = m_initial_num_segments;
		m_segmentlist = (T**)new char[m_segments_capacity * sizeof(T*)];
		memset(m_segmentlist, 0, sizeof(T*) * m_segments_capacity);
	}
}


// --------------------------------------------------------------------------
// ---- TESTING -------------------------------------------------------------
// --------------------------------------------------------------------------


struct pool_test_element_t
{
	pool_test_element_t(int _v1, int _v2): v1(_v1), v2(_v2) {}
	int v1;
	int v2;
};


template<class T>
void pool_t<T>::test()
{
	pool_t<pool_test_element_t> p(2,2);
	pool_test_element_t  e1(3,4);
	pool_test_element_t  e2(5,6);
	pool_test_element_t  e4(9,10);
	pool_test_element_t* e;

	printf("111\n");
	p.append(e1);
	p.append(e2);

	printf("222\n");
	assert(p.size() == 2);

	printf("2221\n");
	pool_test_element_t* e3 = p.append();
	printf("2222\n");
	e3->v1 = 7;
	e3->v2 = 8;
	//*e3 = pool_test_element_t(7, 8);
	printf("2223\n");
	p.append(e4);

	printf("333\n");
	assert(p.size() == 4);

	e = p.get(0);
	assert(e->v1 == 3 && e->v2 == 4);
	e = p.get(3);
	printf("444\n");
	assert(e->v1 == 9 && e->v2 == 10);
	e = p.get(2);
	assert(e->v1 == 7 && e->v2 == 8);
	e = p.get(1);
	printf("555 v1 %i v2 %i\n", e->v1, e->v2);
	assert(e->v1 == 5 && e->v2 == 6);
	printf("555\n");
}


#endif // __POOL_T_H__
