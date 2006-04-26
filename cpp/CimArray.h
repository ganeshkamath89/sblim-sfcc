

#ifndef Sfcc_CimcArray_h
#define Sfcc_CimcArray_h

#include "cimcdt.h"
#include "cimcft.h"

#include "CimData.h"


class CimcArray;
class CimcInstance;
class CimcData;

class CimcArrayIdx {
   friend class CimcArray;
   const CimcArray &ar;
   cimcCount idx;
   CimcArrayIdx(const CimcArray &a, cimcCount i);
public:
   CimcArrayIdx& operator=(const CimcData&);
   CimcData getData() const;
   operator CimcString() const;
   operator const char* () const;
//   operator CimcDateTime() const;
   operator CimcInstance() const;
   operator CimcObjectPath() const;
   operator cimcSint8() const;
   operator cimcSint16() const;
   operator cimcSint32() const;
   operator int() const;
   operator cimcSint64() const;
   operator unsigned char() const;
   operator unsigned short() const;
   operator cimcUint32() const;
   operator unsigned int() const;
   operator cimcUint64() const;
   operator cimcReal64() const;

};


/** This class wraps a cimcData value array.
      Index operations use the [] operator.
      Data extraction uses type conversion operators.
      Extraction operations can be appended to an array indexing operation
      like this:

        CimcString state;

 CimcArray states;

        states=ci.getProperty("States");

 state=states[3];

 Assignment statements use array indexing operations as well:

 states[5]="offline";

       Type mismatches will be signalled by exceptions.
*/

class CimcArray {
   friend class CimcArrayIdx;
   friend class CimcData;
   friend class CimcEnumeration;
   cimcArray *enc;
   void operator=(int x);
   cimcArray *getEnc() const;
   CimcArray(cimcArray *arr);
public:
   CimcArray(cimcCount max, cimcType type);
   CimcArray();
   cimcCount size() const;
   CimcArrayIdx operator[](int idx) const;
};

#endif



