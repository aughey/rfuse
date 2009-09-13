#include "rfuse.h"
#include "filler.h"

void Init_rfuse() {
  VALUE mRFuse=rb_define_module("RFuse");
  file_info_init(mRFuse);
  context_init(mRFuse);
  rfiller_init(mRFuse);
  rfuse_init(mRFuse);
}
