#include <iostream>
#include "aes.h" 
#include <string>
#include <cstdlib>
#include <time.h>
#include <array>
#include <climits>
#include <bitset>
#define SIZEOFFACES 512
#define SIZEOFVOICES 256
class encAlg{
	public:
		const static int floatBitSize = sizeof(float) * CHAR_BIT;
		std::array<float, SIZEOFFACES> faces;
		std::array<float, SIZEOFVOICES> voices;
		std::array<std::bitset<floatBitSize>, SIZEOFFACES> encFaces;
		std::array<std::bitset<floatBitSize>, SIZEOFVOICES> encVoices; 
		// Adds the faces and voices to two arrays
		void alterArray() {
			for(int i = 0; i < SIZEOFFACES; i++) {
				faces[i] = static_cast<float>( rand() )/static_cast<float>( RAND_MAX );
				if(i < 256) {
					voices[i] = static_cast<float>( rand() )/ static_cast<float>( RAND_MAX );
				}
			}	
		}
		// Takes the float value as a refrence then reinterpret casts it as a pointer to a 32 bit integer
		static std::bitset<floatBitSize> createBits(float floatValue) {	
			std::bitset<floatBitSize> bits(*reinterpret_cast<uint32_t*>(&floatValue));	
			return bits;
		}
		// Encryption method. Currently this is XOR encryption. Implement AES256 Encryption 
		void encrypt() {
			for(int i = 0; i < SIZEOFFACES; i++) {
				std::bitset<floatBitSize> floatInBits = createBits(faces[i]);		
				std::bitset<floatBitSize> shiftedBits = floatInBits << 3;
				std::bitset<floatBitSize> xoredBits = floatInBits ^ shiftedBits;
				encFaces[i] = xoredBits;
			} 
			
		}	 
	};
int main() {
	
	encAlg encryptionClass;
	srand (time(NULL));
	encryptionClass.encrypt();	
}
	
