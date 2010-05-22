/* Copyright (C) 2010  John Whitney
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
 * Author: John Whitney <jjw@linuxmail.org>
 */

#include "Python.h"
#include "libbdelta.cpp"

const char *a_global, *b_global;

void *a_read(unsigned place, unsigned num) {
	return (char*)(a_global + place);
}

void *b_read(unsigned place, unsigned num) {
	return (char*)(b_global + place);
}

void dumpContent(char *name, char *s, size_t len) {
	printf("%s: ", name);
	for (int i = 0; i < len; ++i)
		if (isprint(s[i]))
			printf("%c", s[i]);
		else
			printf("%i", s[i]);
	printf("\n\n");
}

PyObject* bdelta_SimpleString(PyObject* self, PyObject* args) {
	Py_UNICODE *a, *b;
	int len_a, len_b;
	int smallestMatch;

	if (!PyArg_ParseTuple(args, "u#u#i", &a, &len_a, &b, &len_b, &smallestMatch))
		return NULL;
	PyObject *a16 = PyUnicode_EncodeUTF16(a, len_a, NULL, -1);
	PyObject *b16 = PyUnicode_EncodeUTF16(b, len_b, NULL, -1);
	
	smallestMatch *= 2;

#ifndef NDEBUG
	dumpContent("String 1", PyString_AsString(a16), len_a*2);
	dumpContent("String 2", PyString_AsString(b16), len_b*2);
#endif
	a_global = (char *)PyString_AsString(a16);
	b_global = (char *)PyString_AsString(b16);
	void *bi = bdelta_init_alg(len_a*2, len_b*2, a_read, b_read);
	int nummatches;
	for (int i = 64; i >= smallestMatch; i/=2)
		nummatches = bdelta_pass(bi, i);

	PyObject *ret = PyTuple_New(nummatches);
	for (int i = 0; i < nummatches; ++i) {
		unsigned p1, p2, num;
		bdelta_getMatch(bi, i, &p1, &p2, &num);
		if ((p1&1)==1 && (p2&1)==1)
			{++p1; ++p2; --num;}
		else if ((p1&1)==1 || (p2&1)==1)
			printf("Unicode Error.\n");
		//printf("%*i, %*i, %*i\n", 10, p1, 10, p2, 10, num);

		PyObject *m = PyTuple_New(3);
		PyTuple_SetItem(m, 0, PyInt_FromLong(p1/2));
		PyTuple_SetItem(m, 1, PyInt_FromLong(p2/2));
		PyTuple_SetItem(m, 2, PyInt_FromLong(num/2));
		PyTuple_SetItem(ret, i, m);
	}
	Py_DECREF(a16);
	Py_DECREF(b16);
	return ret;
}

static PyMethodDef BDeltaMethods[] =
{
     {"bdelta_SimpleString", bdelta_SimpleString, METH_VARARGS, "Get the delta of two strings (returns a list of matches)."},
     {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC

initbdelta_python(void)
{
     (void) Py_InitModule("bdelta_python", BDeltaMethods);
}
