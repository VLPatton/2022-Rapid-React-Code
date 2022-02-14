/******************************************************************************
	Description:	Defines the Vision control class
	Classes:		VisionPacket 
	Project:		2022 Rapid React Robot Code
******************************************************************************/

#ifndef Vision_h
#define Vison_h

const double dAnglePerPixel     = 69.000 / 320.000;
enum DetectionClass : char {
    eBlueHangar = 0x01,
    eRedHangar,
    eHub,
    eRedCargo,
    eBlueCargo
};

class CVisionPacket {
public:
    CVisionPacket();
    CVisionPacket(const char* pPacketArr, unsigned int length);
    ~CVisionPacket();
    void ParseDetections();
    
    char m_nRandVal = 0xFF;
    char m_nDetectionCount = 0xFF;
    
    // Force the compiler to pack tightly in order to properly serialize this out
    #pragma pack(1)
    struct sObjectDetection {
        public:
        short           m_nX;
        short           m_nY;

        short           m_nWidth;
        short           m_nHeight;

        char            m_nConfidence;
        DetectionClass  m_uClass;
        int             m_nDepth;

        sObjectDetection(const char* arr, int offset = 0) {
			m_nX = (short) (arr[offset] << 8 | arr[offset + 1]);
			m_nY = (short) (arr[offset + 2] << 8 | arr[offset + 3]);
			m_nWidth = (short) (arr[offset + 4] << 8 | arr[offset + 5]);
			m_nHeight = (short) (arr[offset + 6] << 8 | arr[offset + 7]);
			m_nConfidence = (arr[offset + 8]);
			m_uClass = (DetectionClass)(arr[offset + 9]);
            
            // Convert the big endian data in the byte array into the int
            m_nDepth = 0;
            for(unsigned int n = 0; n < sizeof(m_nDepth); n++) {
                m_nDepth = (m_nDepth << 8) + arr[(offset + 10) + n];
            }
        }
    };
    #pragma pack(0)
    
    sObjectDetection** m_pDetections;

    // Allocate the extra data buffer at the end
    char* m_pRawPacket;
};

#endif