#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "mdb.h"

static inline PyObject *mdb_valAndOfs(PyObject *object, Py_ssize_t len)
{
  PyObject *tpl= PyTuple_New(2);
  PyTuple_SetItem(tpl, 0, object);
  PyTuple_SetItem(tpl, 1, PyLong_FromUnsignedLong((ulong)len));
  return tpl;
}

/*
  pkt_header
  3 byte integer followed by packet number
  returns tuple (packet_length, packet_number)
*/
static PyObject *
pkt_header(mdbProtocol *self, PyObject *header)
{
    Py_ssize_t size;
    uint8_t *data;

    if (PyBytes_Check(header))
    {
        size= PyBytes_Size(header);
        data= (uint8_t *)PyBytes_AsString(header);
    } else if (PyByteArray_Check(header)) {
        size= PyByteArray_Size(header);
        data= (uint8_t *)PyByteArray_AsString(header);
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected bytes or bytearray");
        return NULL;
    }

    CHECK_SIZE(size, 4);

    return mdb_valAndOfs(PyLong_FromUnsignedLong((unsigned long)uint3korr(data)), (unsigned long)data[3]);
}

static PyObject *
pkt_uint24(mdbProtocol *self, PyObject *header)
{
    Py_ssize_t size;
    uint8_t *data;

    if (PyBytes_Check(header))
    {
        size= PyBytes_Size(header);
        data= (uint8_t *)PyBytes_AsString(header);
    } else if (PyByteArray_Check(header)) {
        size= PyByteArray_Size(header);
        data= (uint8_t *)PyByteArray_AsString(header);
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected bytes or bytearray");
        return NULL;
    }

    CHECK_SIZE(size, 3);

    return PyLong_FromUnsignedLong((unsigned long)uint3korr(data));
}

static PyObject *
pkt_uint32(mdbProtocol *self, PyObject *header)
{
    Py_ssize_t size;
    uint8_t *data;

    if (PyBytes_Check(header))
    {
        size= PyBytes_Size(header);
        data= (uint8_t *)PyBytes_AsString(header);
    } else if (PyByteArray_Check(header)) {
        size= PyByteArray_Size(header);
        data= (uint8_t *)PyByteArray_AsString(header);
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected bytes or bytearray");
        return NULL;
    }

    CHECK_SIZE(size, 4);

    return PyLong_FromUnsignedLong((unsigned long)uint4korr(data));
}

static PyObject *
pkt_uint64(mdbProtocol *self, PyObject *header)
{
    Py_ssize_t size;
    uint8_t *data;

    if (PyBytes_Check(header))
    {
        size= PyBytes_Size(header);
        data= (uint8_t *)PyBytes_AsString(header);
    } else if (PyByteArray_Check(header)) {
        size= PyByteArray_Size(header);
        data= (uint8_t *)PyByteArray_AsString(header);
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected bytes or bytearray");
        return NULL;
    }

    CHECK_SIZE(size, 8);

    return PyLong_FromUnsignedLongLong((unsigned long long)uint8korr(data));
}


static PyObject *
pkt_uint16(mdbProtocol *self, PyObject *header)
{
    Py_ssize_t size;
    uint8_t *data;

    if (PyBytes_Check(header))
    {
        size= PyBytes_Size(header);
        data= (uint8_t *)PyBytes_AsString(header);
    } else if (PyByteArray_Check(header)) {
        size= PyByteArray_Size(header);
        data= (uint8_t *)PyByteArray_AsString(header);
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected bytes or bytearray");
        return NULL;
    }

    CHECK_SIZE(size, 2);

    return PyLong_FromUnsignedLong((unsigned long)uint2korr(data));
}

static inline uint64_t _lencint(uint8_t *buffer, uint8_t *len, uint8_t *retcode, uint64_t remaining)
{
  *retcode= 2; /* 0 = OK, 1 = Null Packet, 2 = Error */

  if (remaining <= 0)
    return 0;

  if (buffer[0] < 0xFB)
  {
    *len= 1;
    *retcode= 0;
    return (uint64_t)buffer[0];
  }
  switch (buffer[0]) {
    case 0xFB:
      *len= 1;
      *retcode= 1;
      return 0;
    case 0xFC:
      if (remaining < 3)
        return 0;
      *retcode= 0;
      *len= 3;
      return uint2korr(buffer + 1);
    case 0xFD:
      if (remaining < 4)
        return 0;
      *retcode= 0;
      *len= 4;
      return uint3korr(buffer + 1);
    case 0xFE:
      if (remaining < 9)
        return 0;
      *retcode= 0;
      *len= 9;
      return uint8korr(buffer + 1);
    default:
      break;
  }
  return 0;
}

static PyObject *
pkt_lencint(mdbProtocol *self, PyObject *header)
{
    Py_ssize_t size;
    PyObject *val= 0;
    uint8_t *data;
    uint8_t  inc;
    uint8_t  retcode;
    uint64_t len;

    if (PyBytes_Check(header))
    {
        size= PyBytes_Size(header);
        data= (uint8_t *)PyBytes_AsString(header);
    } else if (PyByteArray_Check(header)) {
        size= PyByteArray_Size(header);
        data= (uint8_t *)PyByteArray_AsString(header);
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected bytes or bytearray");
        return NULL;
    }

    len= _lencint(data, &inc, &retcode, size);

    switch (retcode) {
        case 2:
            PyErr_SetString(PyExc_TypeError, "readbuffer is corrupted");
            return NULL;
        case 1:
            Py_INCREF(Py_None);
            val= Py_None;
            break;
        case 0:
            val= PyLong_FromUnsignedLongLong(len);
            break;
    }

    return mdb_valAndOfs(val, (Py_ssize_t)inc);
}

static PyObject *
pkt_lencstr(mdbProtocol *self, PyObject **args, Py_ssize_t nargs)
{
    Py_ssize_t size;
    PyObject *val= 0;
    PyObject *tpl= NULL;
    uint8_t *data;
    uint8_t  inc;
    uint8_t  retcode;
    uint64_t len;
    uint64_t total=0;

    if (nargs < 2)
      return NULL;

    if (!(tpl= PyTuple_New(nargs)))
      return NULL;

    if (PyBytes_Check(args[0]))
    {
        size= PyBytes_Size(args[0]);
        data= (uint8_t *)PyBytes_AsString(args[0]);
    } else if (PyByteArray_Check(args[0])) {
        size= PyByteArray_Size(args[0]);
        data= (uint8_t *)PyByteArray_AsString(args[0]);
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected bytes or bytearray");
        return NULL;
    }


    for (uint32_t i=1; i < nargs; i++)
    {
        inc= 0;
        len= _lencint(data + total, &inc, &retcode, size);

        switch (retcode) {
            case 2:
                PyErr_SetString(PyExc_TypeError, "readbuffer is corrupted");
                return NULL;
            case 1:
                Py_INCREF(Py_None);
                val= Py_None;
                break;
            case 0:
                if (args[i] == Py_True)
                    val= PyUnicode_FromStringAndSize((char *)(data + total + inc), len);
                else
                    val= PyBytes_FromStringAndSize((char *)(data + total + inc), len);
                break;
        }
        PyTuple_SetItem(tpl, i-1, val);
        size-= (len + inc);
        total+= (len + inc);
    }
    PyTuple_SetItem(tpl, nargs - 1, PyLong_FromUnsignedLong((unsigned long)total));

    return tpl;
}

static PyObject *pkt_txtrow(mdbProtocol *self, PyObject *const *args, Py_ssize_t nargs)
{
  uint64_t total= 0;
  uint8_t *data;
  Py_ssize_t columns, size;
  PyObject *lst;
  uint8_t  inc;
  uint8_t  retcode;
  uint64_t len;

  if (nargs != 2)
    return NULL;

  if (!PyList_Check(args[1]))
    return NULL;

  columns= PyList_Size(args[1]);

    if (PyBytes_Check(args[0]))
    {
        size= PyBytes_Size(args[0]);
        data= (uint8_t *)PyBytes_AsString(args[0]);
    } else if (PyByteArray_Check(args[0])) {
        size= PyByteArray_Size(args[0]);
        data= (uint8_t *)PyByteArray_AsString(args[0]);
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected bytes or bytearray");
        return NULL;
    }

  if (!(lst= PyTuple_New(columns)))
    return NULL;

  for (uint32_t i=0; i < columns && total < size; i++)
  {
    uint64_t dataLen= 0;
    const char *encoding;
    PyObject *val= NULL;
    PyObject *tpl= PyList_GET_ITEM(args[1], i);
    PyObject *enc= PyTuple_GET_ITEM(tpl, 0);
    PyObject *conv= PyTuple_GET_ITEM(tpl, 1);

    encoding= PyUnicode_AsUTF8(enc);

    len= _lencint(data + total, &inc, &retcode, size);

    switch (retcode) {
        case 2:
            PyErr_SetString(PyExc_TypeError, "readbuffer is corrupted");
            goto corrupted;
        case 1:
            Py_INCREF(Py_None);
            val= Py_None;
            break;
        case 0:
            if (encoding[0] == 'u')
                val= PyUnicode_FromStringAndSize((char *)(data + total + inc), len);
            else
                val= PyBytes_FromStringAndSize((char *)(data + total + inc), len);
            break;
    }



    total+= (len + inc);

    if (conv != Py_None && val && val != Py_None)
    {
      PyObject *converted= NULL;

      if ((converted= PyObject_CallFunctionObjArgs(conv, val, NULL)))
      {
          Py_DECREF(val);
          PyTuple_SetItem(lst, i, converted);
      }
    } else
      PyTuple_SetItem(lst, i, val);
  }
  return mdb_valAndOfs(lst, total);
corrupted:
  if (lst)
  {
    PyList_Clear(lst);
    Py_DECREF(lst);
  }
  return NULL;
}


static PyMethodDef MDBProtocol_methods[] = {
    {"pkt_header", (PyCFunction)pkt_header, METH_O, "Get packet header"},
    {"pkt_uint64", (PyCFunction)pkt_uint64, METH_O, "Get 8-byte unsigned integer"},
    {"pkt_uint32", (PyCFunction)pkt_uint32, METH_O, "Get 4-byte unsigned integer"},
    {"pkt_uint24", (PyCFunction)pkt_uint24, METH_O, "Get 3-byte unsigned integer"},
    {"pkt_uint16", (PyCFunction)pkt_uint16, METH_O, "Get 2-byte unsigned integer"},
    {"pkt_lencint", (PyCFunction)pkt_lencint, METH_O, "Get length encoded integer"},
    {"pkt_lencstr", _PyCFunction_CAST(pkt_lencstr), METH_FASTCALL, "Get length encoded string(s)"},
    {"pkt_txtrow", _PyCFunction_CAST(pkt_txtrow), METH_FASTCALL, "Get row in text format (length encoded data"},
    {NULL}  // Sentinel
};

/* Member-Definitionen für MDBProtocol-Objekt (z.B. für getattr/setattr) */
static PyMemberDef MDBProtocol_members[] = {
    {NULL}  // Sentinel
};

/* Type-Definition für MDBProtocol */
static PyTypeObject MDBProtocolType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "Protocol",
    .tp_doc = "MDB Protocol object",
    .tp_basicsize = sizeof(mdbProtocol),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_methods = MDBProtocol_methods,
    .tp_members = MDBProtocol_members,
};

/* Moduldefinition */
static struct PyModuleDef mdb_protocol_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "mdb",
    .m_doc = "C extension module for MariaDB protocol handling",
    .m_size = -1,
};

/* Initialisierungsfunktion */
PyMODINIT_FUNC
PyInit_mdb(void)
{
    PyObject *module;

    if ((module = PyModule_Create(&mdb_protocol_module)))
    {
        if (PyType_Ready(&MDBProtocolType) < 0)
        {
            Py_DECREF(module);
            return NULL;
        }

        Py_INCREF(&MDBProtocolType);
        if (PyModule_AddObject(module, "Protocol", (PyObject *)&MDBProtocolType) < 0) {
            Py_DECREF(&MDBProtocolType);
            Py_DECREF(module);
            return NULL;
        }
    }
    return module;
}

