
#ifdef PY3LIB
#include "pylib.h"
#include "../src/tutils.h"

void trepy_pattern_free(PyObject* obj)
{
    tre_pattern_free((tre_Pattern*)PyCapsule_GetPointer(obj, "_tre_pattern"));
}

static PyObject* trepy_compile(PyObject *self, PyObject* args)
{
    char* re;
    int flag;
    int err_code;

    if(!PyArg_ParseTuple(args, "si", &re, &flag)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    tre_Pattern* ret = tre_compile(re, flag, &err_code);

    if (ret) {
        PyObject *o = PyCapsule_New(ret, "_tre_pattern", trepy_pattern_free);
        return o;
    } else {
        return PyLong_FromLong(err_code);
    }
}

static _INLINE
PyObject* tre_Match_c2py(tre_Match* m)
{
    int i;
    PyObject* t = PyTuple_New(m->groupnum);

    for (i = 0; i < m->groupnum; i++) {
        PyObject* t2 = PyTuple_New(3);
        if (m->groups[i].name)
            PyTuple_SetItem(t2, 0, PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, m->groups[i].name, m->groups[i].name_len));
        else {
            Py_INCREF(Py_None);
            PyTuple_SetItem(t2, 0, Py_None);
        }

        if (m->groups[i].head != -1) {
            PyTuple_SetItem(t2, 1, PyLong_FromLong(m->groups[i].head));
            PyTuple_SetItem(t2, 2, PyLong_FromLong(m->groups[i].tail));
        } else {
            Py_INCREF(Py_None);
            Py_INCREF(Py_None);
            PyTuple_SetItem(t2, 1, Py_None);
            PyTuple_SetItem(t2, 2, Py_None);
        }
        PyTuple_SetItem(t, i, t2);
    }

    tre_match_free(m);
    return t;
}

static PyObject* trepy_match(PyObject *self, PyObject* args)
{
    tre_Pattern* pattern;
    PyObject* obj;
    char* text;
    int backtrack_limit;

    if (!PyArg_ParseTuple(args, "Osi", &obj, &text, &backtrack_limit))
        return NULL;

    pattern = (tre_Pattern*)PyCapsule_GetPointer(obj, "_tre_pattern");

    tre_Match* m = tre_match(pattern, text, backtrack_limit);
    if (!m->groups) {
        tre_match_free(m);
        Py_INCREF(Py_None);
        return Py_None;
    }

    return tre_Match_c2py(m);
}

static PyMethodDef tre_methods[] ={
    {"_compile", trepy_compile, METH_VARARGS},
    {"_match", trepy_match, METH_VARARGS},
    {NULL, NULL,0,NULL}
};

static struct PyModuleDef module_def ={
    PyModuleDef_HEAD_INIT,
    "_tinyre",
    "Tiny Regex Engine Module",
    -1,
    tre_methods,
};


static PyObject *TinyreError;


PyMODINIT_FUNC PyInit__tinyre()
{
    PyObject *m;
    m = PyModule_Create(&module_def);

    if (m == NULL)
        return NULL;

    TinyreError = PyErr_NewException("tre.error", NULL, NULL);
    Py_INCREF(TinyreError);
    PyModule_AddObject(m, "error", TinyreError);
    return m;
}
#endif

