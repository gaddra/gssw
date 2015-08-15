/*
 *  ssw.h
 *
 *  Created by Mengyao Zhao on 6/22/10.
 *  Copyright 2010 Boston College. All rights reserved.
 *	Version 0.1.4
 *	Last revision by Mengyao Zhao on 01/30/13.
 *
 *	Modified by Ravi Gaddipati to remove traceback.
 *
 */

#ifndef SSW_H
#define SSW_H

#define DEBUG 5

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <smmintrin.h>
#include <sys/time.h>

static uint64_t
get_timestamp ()
{
    struct timeval now;
    gettimeofday (&now, NULL);
    return  now.tv_usec + (uint64_t)now.tv_sec * 1000000;
}

/*!	@typedef	structure of the query profile	*/
struct gssw_profile;
typedef struct gssw_profile gssw_profile;

typedef struct {
    __m128i* pvE;
    __m128i* pvHStore;
} gssw_seed;


/*!	@typedef	structure of the alignment result
	@field	score1	the best alignment score
	@field	score2	sub-optimal alignment score
	@field	ref_begin1	0-based best alignment beginning position on reference;	ref_begin1 = -1 when the best alignment beginning
						position is not available
	@field	ref_end1	0-based best alignment ending position on reference
	@field	read_begin1	0-based best alignment beginning position on read; read_begin1 = -1 when the best alignment beginning
						position is not available
	@field	read_end1	0-based best alignment ending position on read
	@field	read_end2	0-based sub-optimal alignment ending position on read
	@field	cigar	best alignment cigar; stored the same as that in BAM format, high 28 bits: length, low 4 bits: M/I/D (0/1/2);
					cigar = 0 when the best alignment path is not available
	@field	cigarLen	length of the cigar string; cigarLen = 0 when the best alignment path is not available
*/
typedef struct {
	uint16_t score1;
	uint16_t score2;
	int32_t ref_begin1;
	int32_t ref_end1;
	int32_t	read_begin1;
	int32_t read_end1;
	int32_t ref_end2;
    int32_t prevmax;
    gssw_seed seed;
    uint8_t is_byte;
    void* mH;
} gssw_align;

typedef struct {
	uint16_t score;
	int32_t ref;	 //0-based position
	int32_t read;    //alignment ending position on read, 0-based
} gssw_alignment_end;

typedef struct {
    char type;
    uint32_t length;
} gssw_cigar_element;

typedef struct {
    int32_t length;
    gssw_cigar_element* elements;
} gssw_cigar;

struct gssw_profile{
	__m128i* profile_byte;	// 0: none
	__m128i* profile_word;	// 0: none
	const int8_t* read;
	const int8_t* mat;
	int32_t readLen;
	int32_t n;
	uint8_t bias;
};

//struct node;
//typedef struct node s_node;
typedef struct _gssw_node gssw_node;
typedef struct _gssw_node {
    int data;
    uint32_t id;
    char* seq; // sequence
    int8_t* num; // numerical conversion of seq
    int32_t len; // length of sequence
    gssw_node** prev;
    int32_t count_prev;
    gssw_node** next;
    int32_t count_next;
    gssw_align* alignment;
} _gssw_node;

typedef struct {
    gssw_node* node;
    gssw_cigar cigar;
    int32_t ref_begin;
	int32_t ref_end;
	int32_t	read_begin;
	int32_t read_end;
} gssw_node_mapping;

typedef struct {
    gssw_node* node;
    gssw_alignment_end end;
} gssw_node_alignment_end;

typedef struct {
    uint32_t size;
    gssw_node* max_node;
    gssw_node** nodes;
} gssw_graph;

typedef struct {
    gssw_node* node;
    gssw_cigar* cigar;
} gssw_node_cigar;

typedef struct {
    uint32_t length;   // number of nodes traversed
    gssw_node_cigar* elements; // describes traceback
} gssw_graph_cigar;

typedef struct {
    int32_t position; // position in first node
    int16_t score;
    gssw_graph_cigar cigar;
} gssw_graph_mapping;



#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*!	@function	Create the query profile using the query sequence.
	@param	read	pointer to the query sequence; the query sequence needs to be numbers
	@param	readLen	length of the query sequence
	@param	mat	pointer to the substitution matrix; mat needs to be corresponding to the read sequence
	@param	n	the square root of the number of elements in mat (mat has n*n elements)
	@param	score_size	estimated Smith-Waterman score; if your estimated best alignment score is surely < 255 please set 0; if
						your estimated best alignment score >= 255, please set 1; if you don't know, please set 2
	@return	pointer to the query profile structure
	@note	example for parameter read and mat:
			If the query sequence is: ACGTATC, the sequence that read points to can be: 1234142
			Then if the penalty for match is 2 and for mismatch is -2, the substitution matrix of parameter mat will be:
			//A  C  G  T
			  2 -2 -2 -2 //A
			 -2  2 -2 -2 //C
			 -2 -2  2 -2 //G
			 -2 -2 -2  2 //T
			mat is the pointer to the array {2, -2, -2, -2, -2, 2, -2, -2, -2, -2, 2, -2, -2, -2, -2, 2}
*/
gssw_profile* gssw_init (const int8_t* read, const int32_t readLen, const int8_t* mat, const int32_t n, const int8_t score_size);

/*!	@function	Release the memory allocated by function ssw_init.
	@param	p	pointer to the query profile structure
*/
void gssw_init_destroy (gssw_profile* p);

gssw_align* gssw_align_create(void);


// @function	ssw alignment.
/*!	@function	Do Striped Smith-Waterman alignment.
	@param	prof	pointer to the query profile structure
	@param	ref	pointer to the target sequence; the target sequence needs to be numbers and corresponding to the mat parameter of
				function ssw_init
	@param	refLen	length of the target sequence
	@param	weight_gapO	the absolute value of gap open penalty
	@param	weight_gapE	the absolute value of gap extension penalty
	@param	flag	bitwise FLAG; (from high to low) bit 5: when setted as 1, function ssw_align will return the best alignment
					beginning position; bit 6: when setted as 1, if (ref_end1 - ref_begin1 < filterd && read_end1 - read_begin1
					< filterd), (whatever bit 5 is setted) the function will return the best alignment beginning position and
					cigar; bit 7: when setted as 1, if the best alignment score >= filters, (whatever bit 5 is setted) the function
  					will return the best alignment beginning position and cigar; bit 8: when setted as 1, (whatever bit 5, 6 or 7 is
 					setted) the function will always return the best alignment beginning position and cigar. When flag == 0, only
					the optimal and sub-optimal scores and the optimal alignment ending position will be returned.
	@param	filters	score filter: when bit 7 of flag is setted as 1 and bit 8 is setted as 0, filters will be used (Please check the
 					decription of the flag parameter for detailed usage.)
	@param	filterd	distance filter: when bit 6 of flag is setted as 1 and bit 8 is setted as 0, filterd will be used (Please check
					the decription of the flag parameter for detailed usage.)
	@param	maskLen	The distance between the optimal and suboptimal alignment ending position >= maskLen. We suggest to use
					readLen/2, if you don't have special concerns. Note: maskLen has to be >= 15, otherwise this function will NOT
					return the suboptimal alignment information. Detailed description of maskLen: After locating the optimal
					alignment ending position, the suboptimal alignment score can be heuristically found by checking the second
					largest score in the array that contains the maximal score of each column of the SW matrix. In order to avoid
					picking the scores that belong to the alignments sharing the partial best alignment, SSW C library masks the
					reference loci nearby (mask length = maskLen) the best alignment ending position and locates the second largest
					score from the unmasked elements.
	@return	pointer to the alignment result structure
	@note	Whatever the parameter flag is setted, this function will at least return the optimal and sub-optimal alignment score,
			and the optimal alignment ending positions on target and query sequences. If both bit 6 and 7 of the flag are setted
			while bit 8 is not, the function will return cigar only when both criteria are fulfilled. All returned positions are
			0-based coordinate.
*/
gssw_align* gssw_ssw_align (const gssw_profile* prof,
                            const int8_t* ref,
                            int32_t refLen,
                            const uint8_t weight_gapO,
                            const uint8_t weight_gapE,
                            const uint8_t flag,
                            const uint16_t filters,
                            const int32_t filterd,
                            const int32_t maskLen);

// @function	ssw fill.
/*!	@function	Do Striped Smith-Waterman alignment, but do not generate cigar.  Only return best score, filled matrix, and last E vector.
	@param	prof	pointer to the query profile structure
	@param	ref	pointer to the target sequence; the target sequence needs to be numbers and corresponding to the mat parameter of
				function ssw_init
	@param	refLen	length of the target sequence
	@param	weight_gapO	the absolute value of gap open penalty
	@param	weight_gapE	the absolute value of gap extension penalty
	@param	flag	bitwise FLAG; (from high to low) bit 5: when setted as 1, function ssw_align will return the best alignment
					beginning position; bit 6: when setted as 1, if (ref_end1 - ref_begin1 < filterd && read_end1 - read_begin1
					< filterd), (whatever bit 5 is setted) the function will return the best alignment beginning position and
					cigar; bit 7: when setted as 1, if the best alignment score >= filters, (whatever bit 5 is setted) the function
  					will return the best alignment beginning position and cigar; bit 8: when setted as 1, (whatever bit 5, 6 or 7 is
 					setted) the function will always return the best alignment beginning position and cigar. When flag == 0, only
					the optimal and sub-optimal scores and the optimal alignment ending position will be returned.
	@param	filters	score filter: when bit 7 of flag is setted as 1 and bit 8 is setted as 0, filters will be used (Please check the
 					decription of the flag parameter for detailed usage.)
	@param	filterd	distance filter: when bit 6 of flag is setted as 1 and bit 8 is setted as 0, filterd will be used (Please check
					the decription of the flag parameter for detailed usage.)
	@param	maskLen	The distance between the optimal and suboptimal alignment ending position >= maskLen. We suggest to use
					readLen/2, if you don't have special concerns. Note: maskLen has to be >= 15, otherwise this function will NOT
					return the suboptimal alignment information. Detailed description of maskLen: After locating the optimal
					alignment ending position, the suboptimal alignment score can be heuristically found by checking the second
					largest score in the array that contains the maximal score of each column of the SW matrix. In order to avoid
					picking the scores that belong to the alignments sharing the partial best alignment, SSW C library masks the
					reference loci nearby (mask length = maskLen) the best alignment ending position and locates the second largest
					score from the unmasked elements.
	@return	pointer to the alignment result structure
	@note	Whatever the parameter flag is setted, this function will at least return the optimal and sub-optimal alignment score,
			and the optimal alignment ending positions on target and query sequences. If both bit 6 and 7 of the flag are setted
			while bit 8 is not, the function will return cigar only when both criteria are fulfilled. All returned positions are
			0-based coordinate.
*/
gssw_align* gssw_ssw_fill (const gssw_profile* prof,
                           const int8_t* ref,
                           const int32_t refLen,
                           const uint8_t weight_gapO,
                           const uint8_t weight_gapE,
                           const int32_t maskLen,
                           gssw_seed* seed);


/*!	@function	Release the memory allocated by function ssw_align.
	@param	a	pointer to the alignment result structure
*/
void gssw_align_destroy (gssw_align* a);

/*!	@function	Release the memory allocated for mH and pvE in s_align.
	@param	a	pointer to the alignment result structure
*/
void gssw_align_clear_matrix_and_seed (gssw_align* a);

/*! @function       Print score matrix, determines stride from result score
    @param refLen   Reference length.
    @param readLen  Read length.
    @param result   Alignment result.
*/
void gssw_print_score_matrix (const char* ref,
                              int32_t refLen,
                              const char* read,
                              int32_t readLen,
                              gssw_align* alignment,
                              FILE* out);

void gssw_graph_print(gssw_graph* graph);
void gssw_graph_print_stderr(gssw_graph* graph);

    
/*! @function         Return 1 if the alignment is in 16/128bit (byte sized) or 0 if word-sized.
    @param alignment  Alignment structure.
*/
int gssw_is_byte (gssw_align* alignment);

/*! @function         Generate a traceback of the given alignment, using H, E, and F matrices
    @param alignment  Alignment structure.
    @param readPos    Starting position of alignment in reference.
    @param readPos    Starting position of alignment in read.
*/

void gssw_profile_destroy(gssw_profile* prof);
void gssw_seed_destroy(gssw_seed* seed);
gssw_seed* gssw_create_seed_byte(int32_t readLen, gssw_node** prev, int32_t count);
gssw_seed* gssw_create_seed_word(int32_t readLen, gssw_node** prev, int32_t count);

void gssw_cigar_push_back(gssw_cigar* c, char type, uint32_t length);
void gssw_cigar_push_front(gssw_cigar* c, char type, uint32_t length);
void gssw_reverse_cigar(gssw_cigar* c);
void gssw_print_cigar(gssw_cigar* c);
void gssw_cigar_destroy(gssw_cigar* c);

gssw_node* gssw_node_create(int data,
                            const uint32_t id,
                            const char* seq,
                            const int8_t* nt_table,
                            const int8_t* score_matrix);
void gssw_node_destroy(gssw_node* n);
void gssw_node_add_prev(gssw_node* n, gssw_node* m);
void gssw_node_add_next(gssw_node* n, gssw_node* m);
void gssw_nodes_add_edge(gssw_node* n, gssw_node* m);
void gssw_node_del_prev(gssw_node* n, gssw_node* m);
void gssw_node_del_next(gssw_node* n, gssw_node* m);
void gssw_nodes_del_edge(gssw_node* n, gssw_node* m);
void gssw_node_replace_prev(gssw_node* n, gssw_node* m, gssw_node* p);
void gssw_node_replace_next(gssw_node* n, gssw_node* m, gssw_node* p);


gssw_node*
gssw_node_fill (gssw_node* node,
                const gssw_profile* prof,
                const uint8_t weight_gapO,
                const uint8_t weight_gapE,
                const int32_t maskLen,
                const gssw_seed* seed,
                __m128i* pvHStore, __m128i* pvHLoad, __m128i* pvHmax, __m128i* pvE);

gssw_graph*
gssw_graph_fill (gssw_graph* graph,
                 const char* read_seq,
                 const int8_t* nt_table,
                 const int8_t* score_matrix,
                 const uint8_t weight_gapO,
                 const uint8_t weight_gapE,
                 const int32_t maskLen,
                 const int8_t score_size);

gssw_graph* gssw_graph_create(uint32_t size);
int32_t gssw_graph_add_node(gssw_graph* graph,
                            gssw_node* node);
void gssw_graph_clear(gssw_graph* graph);
void gssw_graph_destroy(gssw_graph* graph);
void gssw_graph_print_score_matrices(gssw_graph* graph,
                                     const char* read,
                                     int32_t readLen,
                                     FILE* out);

gssw_graph_mapping* gssw_graph_mapping_create(void);
void gssw_graph_mapping_destroy(gssw_graph_mapping* m);
gssw_graph_cigar* gssw_graph_cigar_create(void);
void gssw_graph_cigar_destroy(gssw_graph_cigar* g);
void gssw_print_graph_cigar(gssw_graph_cigar* g);
void gssw_print_graph_mapping(gssw_graph_mapping* gm);
//void gssw_graph_cigar_to_string(gssw_graph_cigar* g);

// some utility functions
int8_t* gssw_create_score_matrix(int32_t match, int32_t mismatch);
int8_t* gssw_create_nt_table(void);
int8_t* gssw_create_num(const char* seq,
                        const int32_t len,
                        const int8_t* nt_table);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif	// SSW_H
