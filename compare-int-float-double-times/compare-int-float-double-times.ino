//
// ESP int vs float math test
// Evaluates the relative performance of int16, int32, float, double arithmetic (+ * /)
//

extern "C" {
#include "bootloader_random.h"
}
#include <Streaming.h>

// 6000 seems to be as large as we can get for ARRSIZE.  This works out
// to about 96K of array data.
#define ARRSIZE 6000
volatile int64_t dataA[ARRSIZE], dataB[ARRSIZE];
volatile int16_t int16sum;
volatile int32_t int32sum;
volatile int64_t int64sum;
volatile float floatsum;
volatile double doublesum;

int iter;
long tStart, tEnd, dT;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(100);
  Serial << "*** Basic math operation timing test ***\n\n";
  
  // Doing this to randomize things.
  bootloader_random_enable();
  
  iter = 1;
}

// At one point, we randomized test orders to see if this had any impact on the
// timing.  It didn't.  But we'll keep the legacy code here just for grins.
#define RANDOMIZE_TEST_ORDER false
#if RANDOMIZE_TEST_ORDER
#define NTESTS 6
struct TestOrder {
  int rnd;
  int testId;
};
int testOrderCompare(const void* t1, const void* t2) {
  if(((TestOrder*)t1)->rnd < ((TestOrder*)t2)->rnd) return -1;
  if(((TestOrder*)t1)->rnd > ((TestOrder*)t2)->rnd) return 1;
  return 0;
}
#endif // RANDOMIZE_TEST_ORDER

void loop() {
  #if RANDOMIZE_TEST_ORDER
  TestOrder testOrder[6];
  #endif

  // put your main code here, to run repeatedly:
  Serial << "\n*** Iteration " << iter << " starting.  Times are in ns.\n";
  // Generate some random values for the mathops
  esp_fill_random((int64_t*)dataA, ARRSIZE*sizeof(int64_t));
  esp_fill_random((int64_t*)dataB, ARRSIZE*sizeof(int64_t));
  
  #if RANDOMIZE_TEST_ORDER
  // Randomize the test order
  for(int i=0; i<NTESTS; i++) {
    testOrder[i].testId = i;
    esp_fill_random(&testOrder[i].rnd, sizeof(int));
  }
  qsort(testOrder, NTESTS, sizeof(TestOrder), testOrderCompare);
  // run the tests 
  Serial << "\t+\t-\t*\t/\t%\t\n";
  for(int i=0; i<NTESTS; i++) {
    switch(testOrder[i].testId) {
        case 0: fixedTest<int16_t>("int16"); break;
        case 1: fixedTest<int32_t>("int32"); break;
        case 2: fixedTest<uint16_t>("uint16"); break;
        case 3: fixedTest<uint32_t>("unit32"); break;
        case 4: floatTest<float>("float"); break;
        case 5: floatTest<double>("double"); break;
    }
  }
  #else // just do the tests in sequence
  Serial << "\t+\t-\t*\t/\t%\t\n";
  fixedTest<int16_t>("int16");
  fixedTest<int32_t>("int32");
  fixedTest<uint16_t>("uint16");
  fixedTest<uint32_t>("unit32");
  floatTest<float>("float");
  floatTest<double>("double");
  #endif // RANDOMIZE_TEST_ORDER

  // end of cycle
  delay(1000);
  iter++;
}

template<typename T> unsigned long baseTime() {
  // calculate the time for a += b
  // assumes dataA and dataB have been filled with random values
  unsigned long tStart, tEnd;
  tStart = micros();
  for(int i=0; i<ARRSIZE; i++) {
    ((T*)dataA)[i] += ((T*)dataB)[i];
  }
  tEnd = micros();
  return tEnd-tStart;
}
template<typename T> unsigned long addTime(volatile T c) {
  // calculate time for a += b + c
  // Assumes all are volatile
  unsigned long tStart, tEnd;
  tStart = micros();
  for(int i=0; i<ARRSIZE; i++) {
    ((T*)dataA)[i] += ((T*)dataB)[i] + c;
  }
  tEnd = micros();
  return tEnd-tStart;
}
template<typename T> unsigned long subTime(volatile T c) {
  // calculate time for a += b - c
  // Assumes all are volatile
  unsigned long tStart, tEnd;
  tStart = micros();
  for(int i=0; i<ARRSIZE; i++) {
    ((T*)dataA)[i] += ((T*)dataB)[i] - c;
  }
  tEnd = micros();
  return tEnd-tStart;
}
template<typename T> unsigned long mulTime(volatile T c) {
  // calculate time for a += b * c
  // Assumes all are volatile
  unsigned long tStart, tEnd;
  Serial1 << "c is " << c << endl;
  tStart = micros();
  for(int i=0; i<ARRSIZE; i++) {
    ((T*)dataA)[i] += ((T*)dataB)[i] * c;
  }
  tEnd = micros();
  return tEnd-tStart;
}
template<typename T> unsigned long divTime(volatile T c) {
  // calculate time for a += b / c
  // Assumes all are volatile
  unsigned long tStart, tEnd;
  tStart = micros();
  for(int i=0; i<ARRSIZE; i++) {
    ((T*)dataA)[i] += ((T*)dataB)[i] / c;
  }
  tEnd = micros();
  return tEnd-tStart;
}
template<typename T> unsigned long modTime(volatile T c) {
  // calculate time for a += b % c
  // Assumes all are volatile
  unsigned long tStart, tEnd;
  tStart = micros();
  for(int i=0; i<ARRSIZE; i++) {
    ((T*)dataA)[i] += ((T*)dataB)[i] % c;
  }
  tEnd = micros();
  return tEnd-tStart;
}
template<typename T> void fixedTest(char* label) {
  unsigned long tStart, tEnd, tBase;
  unsigned long dtBase, dtAdd, dtSub, dtMul, dtDiv, dtMod;
  volatile T c; // for a += b op c
  esp_fill_random((T*)dataA, ARRSIZE*sizeof(T));
  esp_fill_random((T*)dataB, ARRSIZE*sizeof(T));
  do {
    esp_fill_random((T*)&c, sizeof(T));
  } while (c==0);   // prevent /0
  dtBase = baseTime<T>();
  dtAdd = addTime<T>(c);
  dtSub = subTime<T>(c);
  dtMul = mulTime<T>(c);
  dtDiv = divTime<T>(c);
  dtMod = modTime<T>(c);
  Serial << label << '\t'
    << (dtAdd-dtBase)/(float)ARRSIZE * 1000 << '\t'
    << (dtSub-dtBase)/(float)ARRSIZE * 1000 << '\t'
    << (dtMul-dtBase)/(float)ARRSIZE * 1000 << '\t'
    << (dtDiv-dtBase)/(float)ARRSIZE * 1000 << '\t'
    << (dtMod-dtBase)/(float)ARRSIZE * 1000 << '\t'
    << endl;
}
template<typename T> void floatTest(char* label) {
  unsigned long tStart, tEnd, tBase;
  unsigned long dtBase, dtAdd, dtSub, dtMul, dtDiv;
  volatile T c; // for a += b op c
  int32_t c1, c2;
  esp_fill_random((T*)dataA, ARRSIZE*sizeof(T));
  esp_fill_random((T*)dataB, ARRSIZE*sizeof(T));
  c1 = esp_random();
  do {
    c2 = esp_random();
  } while (c2==0);   // prevent /0
  c = c1/static_cast<T>(c2);
  dtBase = baseTime<T>();
  dtAdd = addTime<T>(c);
  dtSub = subTime<T>(c);
  dtMul = mulTime<T>(c);
  dtDiv = divTime<T>(c);
  Serial << label << '\t'
    << (dtAdd-dtBase)/(float)ARRSIZE * 1000 << '\t'
    << (dtSub-dtBase)/(float)ARRSIZE * 1000 << '\t'
    << (dtMul-dtBase)/(float)ARRSIZE * 1000 << '\t'
    << (dtDiv-dtBase)/(float)ARRSIZE * 1000 << '\t'
    << endl;
}
