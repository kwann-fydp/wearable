#ifndef classifier_h
#define classifier_h

bool classify_spo2(int32_t spo2) {
	if (spo2 < 91) return true;
	else return false;
}

#endif
