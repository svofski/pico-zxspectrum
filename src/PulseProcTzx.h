#pragma once

#include <pico/stdlib.h>
#include "PulseProc.h"
#include <vector>
#include "PulseProcTzxHeader.h"
#include "PulseProcTzxGlue.h"
#include "PulseProcTzxIndex.h"
#include "PulseProcTzxBlock.h"

class PulseProcTzx : public PulseProc {
private:
  std::vector<uint32_t> _bi;
  PulseProcTzxHeader _pptHeader;
  PulseProcTzxGlue _pptGlue;
  PulseProcTzxIndex _pptIndex;
  PulseProcTzxBlock _pptBlock;
  uint32_t _tsPerMs;
  
public:

  PulseProcTzx(PulseProcTap* ppTap);
  
  void init(
    PulseProc *next,
    uint32_t tsPerMs
  );
  
  virtual int32_t advance(
    InputStream *is,
    bool *pstate,
    PulseProc **top
  );
};
