# Chess-Game
This is Chess game developed in c++


Chess Game with SFML

Welcome to this fully-featured 2D chess game built using C++ and the SFML multimedia library. This project brings the classic game of chess to your screen with smooth gameplay, complete chess logic, and an engaging audio-visual experience. Whether you're learning chess, testing strategies, or just looking for a well-coded game example, this implementation has something to offer.
Features Overview

This chess game includes all standard rules and mechanics. You can make every legal chess move including castling, en passant captures, and pawn promotion. The game automatically detects check and checkmate situations, validates moves to prevent illegal plays, and manages turn-based gameplay between white and black. Visually, the game presents a clean chessboard with coordinate labels, highlights for selected pieces and valid moves, and clear indicators for captures and check. When a king is in check, the square lights up in red, and the last move made is subtly highlighted for easy tracking.

Sound plays a big role in the experience. The game includes distinct sound effects for moves, captures, checks, castling, and pawn promotions. Background music can be toggled on or off, and all sounds are programmatically generated, meaning no external sound files are required. You control the game entirely with your mouse—click to select pieces and click again to move them. Keyboard shortcuts let you restart the game, toggle audio settings, or quit easily.
How to Get Started

To run this game, you'll need a C++ compiler and the SFML library installed. First, clone the repository from GitHub. Make sure you have SFML set up on your system; installation guides are available on the official SFML website. You'll also need to provide your own set of chess piece images in PNG format. Place these in an "images" folder with specific filenames like "white-king.png" and "black-pawn.png". Once everything is set up, compile the code linking the necessary SFML modules, and run the executable to start playing.
Playing the Game

Launch the game to begin with white's turn. Click on any of your pieces to see its possible moves highlighted on the board. Green dots indicate empty squares you can move to, while red outlines show squares where you can capture an opponent's piece. Click on a highlighted square to execute the move. Special moves like castling are handled automatically—just move your king two squares toward the rook. For pawn promotion, when a pawn reaches the opposite side of the board, it will automatically become a queen. The game ends when a checkmate is achieved, or a stalemate occurs, with an on-screen message declaring the result.
Project Structure and Technical Notes

The entire game logic is contained within a single C++ file. The board is represented as an 8x8 integer array, with each integer corresponding to a specific piece type. Move generation, check detection, and game state evaluation are all implemented from scratch. The rendering uses SFML's sprite and shape drawing functions, while audio is generated dynamically using sine waves for simple, effective sound effects. The code is designed to be readable and modular, with clear functions for each chess piece's move logic and game state checks.
