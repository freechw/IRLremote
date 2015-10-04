/*
Copyright (c) 2014-2015 NicoHood
See the readme for credit to other people.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

// Include guard
#pragma once

//================================================================================
// Protocol Definitions
//================================================================================

//RawIR
#define RAWIR_BLOCKS 255				// 0-65535
#define RAWIR_TIMEOUT (0xFFFF/2)		// 65535, max timeout
#define RAWIR_TIME_THRESHOLD 10000UL	// 0-32bit

// Determine buffer length datatype
#if (RAWIR_BLOCKS > 255)
#define RAWIR_DATA_T uint16_t
#else
#define RAWIR_DATA_T uint8_t
#endif

//================================================================================
// Decoding Class
//================================================================================

class RawIR : public CIRLData{
public:
	RawIR(void){
		// Empty	
	}
	
	// Hide anything that is inside this class so the user dont accidently uses this class
	template<typename protocol, typename ...protocols>
	friend class CIRLremote;
	
private:
	static inline uint8_t getSingleFlag(void) __attribute__((always_inline));
	static inline bool requiresCheckTimeout(void) __attribute__((always_inline));
	static inline void checkTimeout(void) __attribute__((always_inline));
	static inline bool timeout(void) __attribute__((always_inline));
	static inline bool available(void) __attribute__((always_inline));
	static inline void read(IR_data_t* data) __attribute__((always_inline));
	static inline bool requiresReset(void) __attribute__((always_inline));
	static inline void reset(void) __attribute__((always_inline));
	
	// Decode functions for a single protocol/multiprotocol for less/more accuration
	static inline void decodeSingle(const uint16_t &duration) __attribute__((always_inline));
	static inline void decode(const uint16_t &duration) __attribute__((always_inline));

//protected:
public:
	// Temporary buffer to hold bytes for decoding the protocols
	// not all of them are compiled, only the used ones
	static RAWIR_DATA_T countRawIR;
	static uint16_t dataRawIR[RAWIR_BLOCKS];
};


uint8_t RawIR::getSingleFlag(void){
	return CHANGE;
}


bool RawIR::requiresCheckTimeout(void){
	// Used in this protocol
	return true;
}


void RawIR::checkTimeout(void){
	// This function is executed with interrupts turned off
	if(timeout()){
		// Flag a new input if reading timed out
		IRLProtocol = IR_RAW;
		IRLLastEvent = IRLLastTime;
	}
}


bool RawIR::timeout(void){
	// Check if we have data
	if (countRawIR) {
		// Check if buffer is full
		if (countRawIR == RAWIR_BLOCKS) {
			return true;
		}

		// Check if last reading was a timeout.
		// No problem if count==0 because the above if around prevents this.
		if (dataRawIR[countRawIR - 1] >= RAWIR_TIMEOUT)
		{
			return true;
		}

		// Check if reading timed out and save value.
		// Saving is needed to abord the check above next time.
		uint32_t duration = micros() - IRLLastTime;
		if (duration >= RAWIR_TIMEOUT) {
			dataRawIR[countRawIR++] = RAWIR_TIMEOUT;
			return true;
		}
	}

	// Continue, we can still save into the buffer or the buffer is empty
	return false;
}


bool RawIR::available(void)
{
	// Only return a value if this protocol has new data
	if(IRLProtocol == IR_RAW)
		return true;
	else
		return false;	
}


void RawIR::read(IR_data_t* data){
	// Only (over)write new data if this protocol received any data
	if(IRLProtocol == IR_RAW){
		// TODO analyze data and output something that can be used better for unknown remotes
		data->address = 0;
		data->command = 0;
	}
}


bool RawIR::requiresReset(void){
	// Used in this protocol
	return true;
}


void RawIR::reset(void){
	// Reset protocol for new reading
	countRawIR = 0;
}


void RawIR::decodeSingle(const uint16_t &duration){
	// Save value and increase count
	dataRawIR[countRawIR++] = duration;
	
	// Flag a new input if buffer is full or reading timed out
	if((countRawIR == RAWIR_BLOCKS) || (duration >= RAWIR_TIMEOUT)){
		// Ignore the very first timeout of each reading
		if(countRawIR == 1){
			countRawIR = 0;
		}
		else{
			IRLProtocol = IR_RAW;
		}
	}
}


void RawIR::decode(const uint16_t &duration) {
	// Wait some time after the last protocol.
	// This way it can finish its (possibly ignored) stop bit.
	uint8_t lastProtocol = IRLProtocol | IR_NEW_PROTOCOL;
	if(lastProtocol != IR_RAW && (IRLLastTime - IRLLastEvent < RAWIR_TIME_THRESHOLD))
		return;

	decodeSingle(duration);
}
