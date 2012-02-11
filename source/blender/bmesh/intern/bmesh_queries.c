/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor(s): Joseph Eagar, Geoffrey Bantle, Campbell Barton
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/bmesh/intern/bmesh_queries.c
 *  \ingroup bmesh
 *
 * This file contains functions for answering common
 * Topological and geometric queries about a mesh, such
 * as, "What is the angle between these two faces?" or,
 * "How many faces are incident upon this vertex?" Tool
 * authors should use the functions in this file instead
 * of inspecting the mesh structure directly.
 */

#include <string.h>

#include "bmesh.h"
#include "bmesh_private.h"

#include "BKE_utildefines.h"

#include "BLI_math.h"
#include "BLI_utildefines.h"

#define BM_OVERLAP (1 << 13)

/*
 * BMESH COUNT ELEMENT
 *
 * Return the amount of element of
 * type 'type' in a given bmesh.
 */

int BM_Count_Element(BMesh *bm, const char htype)
{
	if (htype == BM_VERT) return bm->totvert;
	else if (htype == BM_EDGE) return bm->totedge;
	else if (htype == BM_FACE) return bm->totface;

	return 0;
}


/*
 * BMESH VERT IN EDGE
 *
 * Returns whether or not a given vertex is
 * is part of a given edge.
 *
 */

int BM_Vert_In_Edge(BMEdge *e, BMVert *v)
{
	return bmesh_vert_in_edge(e, v);
}

/*
 * BMESH OTHER EDGE IN FACE SHARING A VERTEX
 *
 * Returns an opposing loop that shares the same face.
 *
 */

BMLoop *BM_OtherFaceLoop(BMEdge *e, BMFace *f, BMVert *v)
{
	BMLoop *l_iter;
	BMLoop *l_first;

	l_iter = l_first = BM_FACE_FIRST_LOOP(f);
	
	do {
		if (l_iter->e == e) {
			break;
		}
	} while ((l_iter = l_iter->next) != l_first);
	
	return l_iter->v == v ? l_iter->prev : l_iter->next;
}

/*
 * BMESH VERT IN FACE
 *
 * Returns whether or not a given vertex is
 * is part of a given face.
 *
 */

int BM_Vert_In_Face(BMFace *f, BMVert *v)
{
	BMLoopList *lst;
	BMLoop *l_iter;

	for (lst = f->loops.first; lst; lst = lst->next) {
		l_iter = lst->first;
		do {
			if (l_iter->v == v) {
				return TRUE;
			}
		} while ((l_iter = l_iter->next) != lst->first);
	}

	return FALSE;
}

/*
 * BMESH VERTS IN FACE
 *
 * Compares the number of vertices in an array
 * that appear in a given face
 *
 */
int BM_Verts_In_Face(BMesh *bm, BMFace *f, BMVert **varr, int len)
{
	BMLoopList *lst;
	BMLoop *curloop = NULL;
	int i, count = 0;
	
	for (i = 0; i < len; i++) BMO_SetFlag(bm, varr[i], BM_OVERLAP);
	
	for (lst = f->loops.first; lst; lst = lst->next) {
		curloop = lst->first;

		do {
			if (BMO_TestFlag(bm, curloop->v, BM_OVERLAP))
				count++;

			curloop = curloop->next;
		} while (curloop != lst->first);
	}

	for (i = 0; i < len; i++) BMO_ClearFlag(bm, varr[i], BM_OVERLAP);

	return count;
}

/*
 * BMESH EDGE IN FACE
 *
 * Returns whether or not a given edge is
 * is part of a given face.
 *
 */

int BM_Edge_In_Face(BMFace *f, BMEdge *e)
{
	BMLoop *l_iter;
	BMLoop *l_first;

	l_iter = l_first = BM_FACE_FIRST_LOOP(f);

	do {
		if (l_iter->e == e) {
			return TRUE;
		}
	} while ((l_iter = l_iter->next) != l_first);

	return FALSE;
}

/*
 * BMESH VERTS IN EDGE
 *
 * Returns whether or not two vertices are in
 * a given edge
 *
 */

int BM_Verts_In_Edge(BMVert *v1, BMVert *v2, BMEdge *e)
{
	return bmesh_verts_in_edge(v1, v2, e);
}

/*
 * BMESH GET OTHER EDGEVERT
 *
 * Given a edge and one of its vertices, returns
 * the other vertex.
 *
 */

BMVert *BM_OtherEdgeVert(BMEdge *e, BMVert *v)
{
	return bmesh_edge_getothervert(e, v);
}

/*
 *  BMESH VERT EDGECOUNT
 *
 *	Returns the number of edges around this vertex.
 */

int BM_Vert_EdgeCount(BMVert *v)
{
	return bmesh_disk_count(v);
}

/*
 *  BMESH EDGE FACECOUNT
 *
 *	Returns the number of faces around this edge
 */

int BM_Edge_FaceCount(BMEdge *e)
{
	int count = 0;
	BMLoop *curloop = NULL;

	if (e->l) {
		curloop = e->l;
		do {
			count++;
			curloop = bmesh_radial_nextloop(curloop);
		} while (curloop != e->l);
	}

	return count;
}

/*
 *  BMESH VERT FACECOUNT
 *
 *	Returns the number of faces around this vert
 */

int BM_Vert_FaceCount(BMVert *v)
{
	int count = 0;
	BMLoop *l;
	BMIter iter;

	BM_ITER(l, &iter, NULL, BM_LOOPS_OF_VERT, v) {
		count++;
	}

	return count;
#if 0 //this code isn't working
	BMEdge *curedge = NULL;

	if (v->e) {
		curedge = v->e;
		do {
			if (curedge->l) count += BM_Edge_FaceCount(curedge);
			curedge = bmesh_disk_nextedge(curedge, v);
		} while (curedge != v->e);
	}
	return count;
#endif
}

/*
 *			BMESH WIRE VERT
 *
 *	Tests whether or not the vertex is part of a wire edge.
 *  (ie: has no faces attached to it)
 *
 *  Returns -
 *	1 for true, 0 for false.
 */

int BM_Wire_Vert(BMesh *UNUSED(bm), BMVert *v)
{
	BMEdge *curedge;

	if (v->e == NULL) {
		return FALSE;
	}
	
	curedge = v->e;
	do {
		if (curedge->l) {
			return FALSE;
		}

		curedge = bmesh_disk_nextedge(curedge, v);
	} while (curedge != v->e);

	return TRUE;
}

/*
 *			BMESH WIRE EDGE
 *
 *	Tests whether or not the edge is part of a wire.
 *  (ie: has no faces attached to it)
 *
 *  Returns -
 *	1 for true, 0 for false.
 */

int BM_Wire_Edge(BMesh *UNUSED(bm), BMEdge *e)
{
	return (e->l) ? FALSE : TRUE;
}

/*
 *			BMESH NONMANIFOLD VERT
 *
 *  A vertex is non-manifold if it meets the following conditions:
 *    1: Loose - (has no edges/faces incident upon it)
 *    2: Joins two distinct regions - (two pyramids joined at the tip)
 *    3: Is part of a non-manifold edge (edge with more than 2 faces)
 *    4: Is part of a wire edge
 *
 *  Returns -
 *	1 for true, 0 for false.
 */

int BM_Nonmanifold_Vert(BMesh *UNUSED(bm), BMVert *v)
{
	BMEdge *e, *oe;
	BMLoop *l;
	int len, count, flag;

	if (v->e == NULL) {
		/* loose vert */
		return TRUE;
	}

	/* count edges while looking for non-manifold edges */
	oe = v->e;
	for (len = 0, e = v->e; e != oe || (e == oe && len == 0); len++, e = bmesh_disk_nextedge(e, v)) {
		if (e->l == NULL) {
			/* loose edge */
			return TRUE;
		}

		if (bmesh_radial_length(e->l) > 2) {
			/* edge shared by more than two faces */
			return TRUE;
		}
	}

	count = 1;
	flag = 1;
	e = NULL;
	oe = v->e;
	l = oe->l;
	while (e != oe) {
		l = (l->v == v) ? l->prev : l->next;
		e = l->e;
		count++; /* count the edges */

		if (flag && l->radial_next == l) {
			/* we've hit the edge of an open mesh, reset once */
			flag = 0;
			count = 1;
			oe = e;
			e = NULL;
			l = oe->l;
		}
		else if (l->radial_next == l) {
			/* break the loop */
			e = oe;
		}
		else {
			l = l->radial_next;
		}
	}

	if (count < len) {
		/* vert shared by multiple regions */
		return TRUE;
	}

	return FALSE;
}

/*
 *			BMESH NONMANIFOLD EDGE
 *
 *	Tests whether or not this edge is manifold.
 *  A manifold edge either has 1 or 2 faces attached
 *	to it.
 *
 *  Returns -
 *	1 for true, 0 for false.
 */

int BM_Nonmanifold_Edge(BMesh *UNUSED(bm), BMEdge *e)
{
	int count = BM_Edge_FaceCount(e);
	if (count != 2 && count != 1) {
		return TRUE;
	}
	return FALSE;
}

/*
 *			BMESH BOUNDARY EDGE
 *
 *	Tests whether or not an edge is on the boundary
 *  of a shell (has one face associated with it)
 *
 *  Returns -
 *	1 for true, 0 for false.
 */

int BM_Boundary_Edge(BMEdge *e)
{
	int count = BM_Edge_FaceCount(e);
	if (count == 1) {
		return TRUE;
	}
	return FALSE;
}

/*
 *			BMESH FACE SHAREDEDGES
 *
 *  Counts the number of edges two faces share (if any)
 *
 *  BMESH_TODO:
 *    Move this to structure, and wrap.
 *
 *  Returns -
 *	Integer
 */

int BM_Face_Share_Edges(BMFace *f1, BMFace *f2)
{
	BMLoop *l_iter;
	BMLoop *l_first;
	int count = 0;
	
	l_iter = l_first = BM_FACE_FIRST_LOOP(f1);
	do {
		if (bmesh_radial_find_face(l_iter->e, f2)) {
			count++;
		}
	} while ((l_iter = l_iter->next) != l_first);

	return count;
}

/*
 *
 *           BMESH EDGE SHARE FACES
 *
 *	Tests to see if e1 shares any faces with e2
 *
 */

int BM_Edge_Share_Faces(BMEdge *e1, BMEdge *e2)
{
	BMLoop *l;
	BMFace *f;

	if (e1->l && e2->l) {
		l = e1->l;
		do {
			f = l->f;
			if (bmesh_radial_find_face(e2, f)) {
				return TRUE;
			}
			l = l->radial_next;
		} while (l != e1->l);
	}
	return FALSE;
}

/**
 *
 *           BMESH EDGE SHARE A VERTEX
 *
 *	Tests to see if e1 shares a vertex with e2
 *
 */

int BM_Edge_Share_Vert(struct BMEdge *e1, struct BMEdge *e2)
{
	return (e1->v1 == e2->v1 ||
	        e1->v1 == e2->v2 ||
	        e1->v2 == e2->v1 ||
	        e1->v2 == e2->v2);
}

/**
 *
 *           BMESH EDGE ORDERED VERTS
 *
 *	Returns the verts of an edge as used in a face
 *  if used in a face at all, otherwise just assign as used in the edge.
 *
 *  Useful to get a determanistic winding order when calling
 *  BM_Make_Ngon() on an arbitrary array of verts,
 *  though be sure to pick an edge which has a face.
 *
 */

void BM_Edge_OrderedVerts(BMEdge *edge, BMVert **r_v1, BMVert **r_v2)
{
	if ( (edge->l == NULL) ||
	     ( ((edge->l->prev->v == edge->v1) && (edge->l->v == edge->v2)) ||
	       ((edge->l->v == edge->v1) && (edge->l->next->v == edge->v2)) )
	     )
	{
		*r_v1 = edge->v1;
		*r_v2 = edge->v2;
	}
	else {
		*r_v1 = edge->v2;
		*r_v2 = edge->v1;
	}
}

/**
 *			BMESH FACE ANGLE
 *
 *  Calculates the angle between two faces. Assumes
 *  That face normals are correct.
 *
 *  Returns -
 *	Float.
 */

float BM_Face_Angle(BMesh *UNUSED(bm), BMEdge *e)
{
	if (BM_Edge_FaceCount(e) == 2) {
		BMLoop *l1 = e->l;
		BMLoop *l2 = e->l->radial_next;
		return acosf(dot_v3v3(l1->f->no, l2->f->no));
	}
	else {
		return (float)M_PI / 2.0f; /* acos(0.0) */
	}
}

/*
 * BMESH EXIST FACE OVERLAPS
 *
 * Given a set of vertices (varr), find out if
 * all those vertices overlap an existing face.
 *
 * Returns:
 * 0 for no overlap
 * 1 for overlap
 *
 *
 */

int BM_Exist_Face_Overlaps(BMesh *bm, BMVert **varr, int len, BMFace **overlapface)
{
	BMIter vertfaces;
	BMFace *f;
	int i, amount;

	if (overlapface) *overlapface = NULL;

	for (i = 0; i < len; i++) {
		f = BMIter_New(&vertfaces, bm, BM_FACES_OF_VERT, varr[i]);
		while (f) {
			amount = BM_Verts_In_Face(bm, f, varr, len);
			if (amount >= len) {
				if (overlapface) *overlapface = f;
				return TRUE;
			}
			f = BMIter_Step(&vertfaces);
		}
	}
	return FALSE;
}

/*
 * BMESH FACE EXISTS
 *
 * Given a set of vertices (varr), find out if
 * there is a face with exactly those vertices
 * (and only those vertices).
 *
 * Returns:
 * 0 for no face found
 * 1 for face found
 */

int BM_Face_Exists(BMesh *bm, BMVert **varr, int len, BMFace **existface)
{
	BMIter vertfaces;
	BMFace *f;
	int i, amount;

	if (existface) *existface = NULL;

	for (i = 0; i < len; i++) {
		f = BMIter_New(&vertfaces, bm, BM_FACES_OF_VERT, varr[i]);
		while (f) {
			amount = BM_Verts_In_Face(bm, f, varr, len);
			if (amount == len && amount == f->len) {
				if (existface) *existface = f;
				return TRUE;
			}
			f = BMIter_Step(&vertfaces);
		}
	}
	return FALSE;
}
