# MAPLE

## Overview
Maple ORAM is a framework designed to prevent the leakage of data access patterns, preserving the privacy of sensitive and encrypted data. It builds upon the concept of Oblivious Random Access Machine (ORAM) and addresses the high end-to-end access delay in the original ORAM scheme, making it more applicable in many related fields, especially encrypted databases. This project was created for Virginia Tech's 11th Annual Hackathon.

## Installation
Before installing Maple ORAM, ensure that you have cmake, make, gmp, NTL, zmq installed on your Linux system. Your C++ compiler should support C++ 17.

To install Maple ORAM:
1. Clone the repository.
2. Navigate to the project directory.
3. Create a new directory named 'build' and navigate into it: `mkdir build && cd build`
4. Run `cmake ..`
5. Run `make`

This will create an executable file named 'MAPLE_v2'.

## Usage
To use Maple ORAM, follow these steps:
1. Open three 3 terminal windows. Start three servers by running './MAPLE_v2'. Choose 2 to select the server as the starting point.  Then choose the different servers with options 1, 2, and 3.
2. In a new window, launch the client using './MAPLE_v2' and choose 1 to set it as the starting point for the client.

## Features
- Prevents leakage of data access patterns.
- Addresses high end-to-end access delay in original ORAM scheme.
- Utilizes cryptographic techniques such as Fully Homomorphic Encryption (FHE), Partially Homomorphic Encryption (PHE), XOR, and Shamir Secret Sharing (SSS).

## Contributing
Provide instructions on how others can contribute to your project.

## License
Include information about the license.

## Contact
Include contact information for people to reach out if they have questions or want to collaborate.

