#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>  
#include <string>

using namespace std;
using namespace sf;

// Constants for the core board area
const int boardSize = 8;
const float BOARD_PIXELS = 800.0f;
const float LABEL_MARGIN = 50.0f;
bool whiteTurn = true;
bool gameOver = false;
bool gameStarted = false;
string gameResult = "";

const float VIRTUAL_WIDTH = BOARD_PIXELS + (LABEL_MARGIN * 2.0f);
const float VIRTUAL_HEIGHT = BOARD_PIXELS + (LABEL_MARGIN * 2.0f);
const float squareSize = BOARD_PIXELS / boardSize;

// Colors
const Color lightSquareColor(240, 217, 181);
const Color darkSquareColor(92, 64, 51);
const Color highlightColor(255, 255, 150, 200);
const Color moveDotColor(100, 255, 100, 180);
const Color captureColor(255, 100, 100);


// Integer constants for Piece Types
const int NONE = 0, W_KING = 1, W_QUEEN = 2, W_BISHOP = 3, W_KNIGHT = 4, W_ROOK = 5;
const int W_PAWN = 6, B_KING = 7, B_QUEEN = 8, B_BISHOP = 9, B_KNIGHT = 10, B_ROOK = 11, B_PAWN = 12;

// Globals for selection/highlighting
int selectedRow = -1;
int selectedCol = -1;
int selectedPieceType = NONE;
bool highlightMovesArr[8][8] = { false };

// For En Passant
int enPassantTargetRow = -1;
int enPassantTargetCol = -1;
bool enPassantPossible = false;

// For Pawn Promotion
bool pawnPromotionPending = false;
int promotionRow = -1;
int promotionCol = -1;
int promotionPawnType = NONE;

// For Castling
bool whiteKingMoved = false, blackKingMoved = false;
bool whiteRookKingMoved = false, whiteRookQueenMoved = false;
bool blackRookKingMoved = false, blackRookQueenMoved = false;

// For Check
bool whiteInCheck = false, blackInCheck = false;
int whiteKingRow = 7, whiteKingCol = 4;
int blackKingRow = 0, blackKingCol = 4;

// Parallel arrays to store move history
const int MAX_MOVES = 1000;  // Maximum moves in a game
int moveHistory_fromRow[MAX_MOVES];
int moveHistory_fromCol[MAX_MOVES];
int moveHistory_toRow[MAX_MOVES];
int moveHistory_toCol[MAX_MOVES];
int moveHistory_pieceMoved[MAX_MOVES];
int moveHistory_pieceCaptured[MAX_MOVES];
int moveHistoryCount = 0;  // Current number of moves stored

// Sound system
bool soundsEnabled = true;
bool musicEnabled = true;

// Sound buffers
SoundBuffer captureSoundBuffer;
SoundBuffer moveSoundBuffer;
SoundBuffer checkSoundBuffer;
SoundBuffer castleSoundBuffer;
SoundBuffer promoteSoundBuffer;
SoundBuffer gameStartSoundBuffer;
SoundBuffer gameEndSoundBuffer;
SoundBuffer illegalMoveSoundBuffer;

// Sound objects
Sound captureSound;
Sound moveSound;
Sound checkSound;
Sound castleSound;
Sound promoteSound;
Sound gameStartSound;
Sound gameEndSound;
Sound illegalMoveSound;

// Music
Music backgroundMusic;

// Global font object
Font globalFont;

// Board representation
int board[8][8];
Sprite pieceSprites[8][8];
Texture pieceTextures[12];

// Function prototypes
void setupView(RenderWindow& window, View& view);
void drawBoard(RenderWindow& window);
void drawCellReferences(RenderWindow& window);
void drawGameStatus(RenderWindow& window);
void drawMoveHistory(RenderWindow& window);
void handleMouseClick(const Event& event, const RenderWindow& window);
bool loadTextures();
bool loadSounds();
void initializeSprites();
void initializeBoard();
void promotePawn(int promotionType);
void updateSpritePosition(int row, int col);
void playSoundEffect(Sound& sound);
void generateBeepSound(SoundBuffer& buffer, float frequency, float duration);

// Movement helpers
bool insideBoard(int r, int c) { return r >= 0 && r < 8 && c >= 0 && c < 8; }
void clearHighlights() { for (int r = 0; r < 8; ++r) for (int c = 0; c < 8; ++c) highlightMovesArr[r][c] = false; }
bool isWhite(int p) { return p >= W_KING && p <= W_PAWN; }
bool isBlack(int p) { return p >= B_KING && p <= B_PAWN; }
bool isEnemyPiece(int target, int me) {
    return (isWhite(me) && isBlack(target)) || (isBlack(me) && isWhite(target));
}

// Check detection
bool isSquareAttacked(int row, int col, bool byWhite);
bool isInCheck(bool whiteKing);
void updateKingPosition();

// Move generation with check validation
void genRookMoves(int r, int c);
void genBishopMoves(int r, int c);
void genKnightMoves(int r, int c);
void genKingMoves(int r, int c);
void genPawnMoves(int r, int c);
void genQueenMoves(int r, int c);
void generateValidMoves(int r, int c);

// Game state checking
bool hasValidMoves(bool forWhite);
bool isCheckmate(bool forWhite);
bool isStalemate(bool forWhite);

// Move execution
void movePiece(int sr, int sc, int tr, int tc);
void performCastling(bool kingside);

int main() {
    VideoMode desktopMode = VideoMode::getDesktopMode();
    RenderWindow window(desktopMode, "Chess Game By Subhan Ali", Style::Fullscreen);

    View view(FloatRect(0.f, 0.f, VIRTUAL_WIDTH, VIRTUAL_HEIGHT));
    setupView(window, view);

    if (!globalFont.loadFromFile("C:/Windows/Fonts/Arial.ttf")) {
        cerr << "Error loading font file!" << endl;
    }

    if (!loadTextures()) return 1;

    // Load sounds
    if (loadSounds()) {
        cout << "All sounds loaded successfully!" << endl;
    }
    else {
        cout << "Some sounds failed to load, using generated sounds instead" << endl;
    }

    initializeBoard();
    initializeSprites();

    // Start background music if available
    if (musicEnabled) {
        backgroundMusic.setLoop(true);
        backgroundMusic.setVolume(30);
    }

    // Play game start sound
    playSoundEffect(gameStartSound);

    Clock clock;
    gameStarted = true;

    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) window.close();
            else if (event.type == Event::MouseButtonPressed && !gameOver)
                handleMouseClick(event, window);
            else if (event.type == Event::Resized) setupView(window, view);
            else if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Escape) window.close();
                else if (event.key.code == Keyboard::R && gameOver) {
                    // Restart game
                    gameOver = false;
                    gameStarted = true;
                    whiteTurn = true;
                    gameResult = "";
                    initializeBoard();
                    initializeSprites();
                    moveHistoryCount = 0;
                    whiteInCheck = blackInCheck = false;
                    playSoundEffect(gameStartSound);
                }
                else if (event.key.code == Keyboard::M) {
                    // Toggle music
                    musicEnabled = !musicEnabled;
                    if (musicEnabled) {
                        backgroundMusic.play();
                    }
                    else {
                        backgroundMusic.pause();
                    }
                }
                else if (event.key.code == Keyboard::S) {
                    // Toggle sound effects
                    soundsEnabled = !soundsEnabled;
                }
            }
        }

        window.clear(Color::Black);
        window.setView(view);
        drawBoard(window);
        drawCellReferences(window);
        drawGameStatus(window);
        //drawMoveHistory(window);

        // Draw all sprites
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if (board[r][c] != NONE) {
                    window.draw(pieceSprites[r][c]);
                }
            }
        }

        window.display();
    }
    return 0;
}

void generateBeepSound(SoundBuffer& buffer, float frequency, float duration) {
    // Generate a simple beep sound
    const unsigned sampleRate = 44100;
    const unsigned sampleCount = static_cast<unsigned>(sampleRate * duration);
    vector<Int16> samples(sampleCount);

    for (unsigned i = 0; i < sampleCount; ++i) {
        samples[i] = static_cast<Int16>(32767 * sin(2 * 3.14159 * frequency * i / sampleRate));
    }

    buffer.loadFromSamples(&samples[0], sampleCount, 1, sampleRate);
}

bool loadSounds() {
    bool allLoaded = true;

    // Generate sounds programmatically if files don't exist
    cout << "Generating sound effects..." << endl;

    // Capture sound - sharp beep
    generateBeepSound(captureSoundBuffer, 800, 0.1f);
    captureSound.setBuffer(captureSoundBuffer);
    captureSound.setVolume(80);

    // Move sound - soft beep
    generateBeepSound(moveSoundBuffer, 400, 0.08f);
    moveSound.setBuffer(moveSoundBuffer);
    moveSound.setVolume(60);

    // Check sound - urgent beep
    generateBeepSound(checkSoundBuffer, 600, 0.15f);
    checkSound.setBuffer(checkSoundBuffer);
    checkSound.setVolume(90);

    // Castle sound - two tones
    generateBeepSound(castleSoundBuffer, 500, 0.2f);
    castleSound.setBuffer(castleSoundBuffer);
    castleSound.setVolume(70);

    // Promote sound - rising tone
    generateBeepSound(promoteSoundBuffer, 300, 0.25f);
    promoteSound.setBuffer(promoteSoundBuffer);
    promoteSound.setVolume(80);

    // Game start sound - fanfare (multiple frequencies)
    generateBeepSound(gameStartSoundBuffer, 600, 0.5f);
    gameStartSound.setBuffer(gameStartSoundBuffer);
    gameStartSound.setVolume(80);

    // Game end sound - descending tone
    generateBeepSound(gameEndSoundBuffer, 200, 0.6f);
    gameEndSound.setBuffer(gameEndSoundBuffer);
    gameEndSound.setVolume(80);

    // Illegal move sound - low buzz
    generateBeepSound(illegalMoveSoundBuffer, 150, 0.1f);
    illegalMoveSound.setBuffer(illegalMoveSoundBuffer);
    illegalMoveSound.setVolume(60);

    cout << "All sound effects generated successfully!" << endl;
    return true;
}

void playSoundEffect(Sound& sound) {
    if (soundsEnabled) {
        sound.play();
    }
}

void drawMoveHistory(RenderWindow& window) {
    if (globalFont.getInfo().family == "" || moveHistoryCount == 0) return;

    Text historyText;
    historyText.setFont(globalFont);
    historyText.setCharacterSize(16);
    historyText.setFillColor(Color::White);
    historyText.setPosition(VIRTUAL_WIDTH - 220, 50);

    string historyStr = "Move History:\n";
    int startIdx = (moveHistoryCount - 6 > 0) ? moveHistoryCount - 6 : 0; // Show last 6 moves

    for (int i = startIdx; i < moveHistoryCount; i++) {
        int pieceMoved = moveHistory_pieceMoved[i];
        string pieceName = "";

        // Determine piece name
        if (pieceMoved == W_KING || pieceMoved == B_KING) pieceName = "K";
        else if (pieceMoved == W_QUEEN || pieceMoved == B_QUEEN) pieceName = "Q";
        else if (pieceMoved == W_ROOK || pieceMoved == B_ROOK) pieceName = "R";
        else if (pieceMoved == W_BISHOP || pieceMoved == B_BISHOP) pieceName = "B";
        else if (pieceMoved == W_KNIGHT || pieceMoved == B_KNIGHT) pieceName = "N";
        else if (pieceMoved == W_PAWN || pieceMoved == B_PAWN) pieceName = "";
        // No default needed since pieceName already initialized to ""

        char fromFile = 'a' + moveHistory_fromCol[i];
        int fromRank = 8 - moveHistory_fromRow[i];
        char toFile = 'a' + moveHistory_toCol[i];
        int toRank = 8 - moveHistory_toRow[i];

        string moveNotation = pieceName + string(1, fromFile) + to_string(fromRank);
        if (moveHistory_pieceCaptured[i] != NONE) {
            moveNotation += "x";
        }
        else {
            moveNotation += "-";
        }
        moveNotation += string(1, toFile) + to_string(toRank);

        // Add move number for white moves
        if (i % 2 == 0) {
            historyStr += to_string(i / 2 + 1) + ". " + moveNotation;
        }
        else {
            historyStr += " " + moveNotation + "\n";
        }
    }

    // Add newline if last move was white
    if (moveHistoryCount % 2 == 1) {
        historyStr += "\n";
    }

    historyText.setString(historyStr);
    window.draw(historyText);
}

void setupView(RenderWindow& window, View& view) {
    unsigned int windowWidth = window.getSize().x;
    unsigned int windowHeight = window.getSize().y;
    float scaleX = (float)windowWidth / VIRTUAL_WIDTH;
    float scaleY = (float)windowHeight / VIRTUAL_HEIGHT;
    float scale = std::min(scaleX, scaleY);
    float newWidth = VIRTUAL_WIDTH * scale;
    float newHeight = VIRTUAL_HEIGHT * scale;
    float posX = (windowWidth - newWidth) / 2.0f;
    float posY = (windowHeight - newHeight) / 2.0f;
    view.setViewport(FloatRect(posX / windowWidth, posY / windowHeight, newWidth / windowWidth, newHeight / windowHeight));
}

void drawBoard(RenderWindow& window) {
    RectangleShape square(Vector2f(squareSize, squareSize));
    RectangleShape captureBox(Vector2f(squareSize - 6, squareSize - 6));
    captureBox.setFillColor(Color::Transparent);
    captureBox.setOutlineColor(captureColor);
    captureBox.setOutlineThickness(4);

    // Check highlight
    RectangleShape checkHighlight(Vector2f(squareSize, squareSize));
    checkHighlight.setFillColor(Color(255, 100, 100, 150));

    // Last move highlight
    RectangleShape lastMoveHighlight(Vector2f(squareSize, squareSize));
    lastMoveHighlight.setFillColor(Color(100, 200, 255, 100));

    for (int i = 0; i < boardSize; ++i) {
        for (int j = 0; j < boardSize; ++j) {
            square.setFillColor((i + j) % 2 == 0 ? lightSquareColor : darkSquareColor);

            // Highlight last move
            if (moveHistoryCount > 0) {
                int lastIdx = moveHistoryCount - 1;
                if ((i == moveHistory_fromRow[lastIdx] && j == moveHistory_fromCol[lastIdx]) ||
                    (i == moveHistory_toRow[lastIdx] && j == moveHistory_toCol[lastIdx])) {
                    lastMoveHighlight.setPosition(j * squareSize + LABEL_MARGIN, i * squareSize + LABEL_MARGIN);
                    window.draw(lastMoveHighlight);
                }
            }

            // Highlight king in check
            if (whiteInCheck && i == whiteKingRow && j == whiteKingCol) {
                checkHighlight.setPosition(j * squareSize + LABEL_MARGIN, i * squareSize + LABEL_MARGIN);
                window.draw(checkHighlight);
            }
            if (blackInCheck && i == blackKingRow && j == blackKingCol) {
                checkHighlight.setPosition(j * squareSize + LABEL_MARGIN, i * squareSize + LABEL_MARGIN);
                window.draw(checkHighlight);
            }

            // Selected square
            if (i == selectedRow && j == selectedCol) {
                square.setFillColor(highlightColor);
            }

            square.setPosition(j * squareSize + LABEL_MARGIN, i * squareSize + LABEL_MARGIN);
            window.draw(square);

            // Move highlights
            if (highlightMovesArr[i][j]) {
                bool isCapture = board[i][j] != NONE;

                if (isCapture) {
                    captureBox.setPosition(j * squareSize + LABEL_MARGIN + 3, i * squareSize + LABEL_MARGIN + 3);
                    window.draw(captureBox);
                }
                else {
                    CircleShape dot(squareSize / 8.f);
                    dot.setFillColor(moveDotColor);
                    dot.setOrigin(dot.getRadius(), dot.getRadius());
                    dot.setPosition(j * squareSize + LABEL_MARGIN + squareSize / 2.f,
                        i * squareSize + LABEL_MARGIN + squareSize / 2.f);
                    window.draw(dot);
                }
            }
        }
    }
}

void drawCellReferences(RenderWindow& window) {
    if (globalFont.getInfo().family == "") return;
    Text text;
    text.setFont(globalFont);
    text.setCharacterSize(20);
    text.setFillColor(Color::White);

    for (int i = 0; i < 8; ++i) {
        text.setString(static_cast<char>('A' + i));
        text.setPosition(LABEL_MARGIN + i * squareSize + squareSize / 2.f - text.getLocalBounds().width / 2.f,
            BOARD_PIXELS + LABEL_MARGIN + 5.f);
        window.draw(text);
    }

    for (int i = 0; i < 8; ++i) {
        text.setString(std::to_string(8 - i));
        text.setPosition(5.f, LABEL_MARGIN + i * squareSize + squareSize / 2.f - text.getLocalBounds().height / 2.f);
        window.draw(text);
    }
}

void drawGameStatus(RenderWindow& window) {
    if (globalFont.getInfo().family == "") return;

    Text statusText;
    statusText.setFont(globalFont);
    statusText.setCharacterSize(24);
    statusText.setFillColor(Color::White);
    statusText.setPosition(LABEL_MARGIN, 10);

    if (gameOver) {
        statusText.setString("Game Over: " + gameResult + " (Press R to Restart)");
    }
    else {
        string turn = whiteTurn ? "White's Turn" : "Black's Turn";
        string check = "";
        if (whiteInCheck) check = " - White in CHECK!";
        if (blackInCheck) check = " - Black in CHECK!";
        statusText.setString(turn + check);

        // Controls info
        Text controlsText;
        controlsText.setFont(globalFont);
        controlsText.setCharacterSize(16);
        controlsText.setFillColor(Color::Yellow);
        controlsText.setPosition(LABEL_MARGIN, 40);
        controlsText.setString("Controls: M - Music | S - Sounds | R - Restart | ESC - Quit");
        window.draw(controlsText);

        // Audio status
        Text audioText;
        audioText.setFont(globalFont);
        audioText.setCharacterSize(14);
        audioText.setFillColor(Color::Green);
        audioText.setPosition(LABEL_MARGIN, 65);
        audioText.setString("Audio: Music " + string(musicEnabled ? "ON" : "OFF") +
            " | Sounds " + string(soundsEnabled ? "ON" : "OFF"));
        window.draw(audioText);
    }

    window.draw(statusText);
}

bool loadTextures() {
    // Load white pieces
    if (!pieceTextures[W_KING - 1].loadFromFile("images/white-king.png")) {
        cerr << "Failed to load white king texture!" << endl;
        return false;
    }
    if (!pieceTextures[W_QUEEN - 1].loadFromFile("images/white-queen.png")) {
        cerr << "Failed to load white queen texture!" << endl;
        return false;
    }
    if (!pieceTextures[W_BISHOP - 1].loadFromFile("images/white-bishop.png")) {
        cerr << "Failed to load white bishop texture!" << endl;
        return false;
    }
    if (!pieceTextures[W_KNIGHT - 1].loadFromFile("images/white-knight.png")) {
        cerr << "Failed to load white knight texture!" << endl;
        return false;
    }
    if (!pieceTextures[W_ROOK - 1].loadFromFile("images/white-rook.png")) {
        cerr << "Failed to load white rook texture!" << endl;
        return false;
    }
    if (!pieceTextures[W_PAWN - 1].loadFromFile("images/white-pawn.png")) {
        cerr << "Failed to load white pawn texture!" << endl;
        return false;
    }

    // Load black pieces
    if (!pieceTextures[B_KING - 1].loadFromFile("images/black-king.png")) {
        cerr << "Failed to load black king texture!" << endl;
        return false;
    }
    if (!pieceTextures[B_QUEEN - 1].loadFromFile("images/black-queen.png")) {
        cerr << "Failed to load black queen texture!" << endl;
        return false;
    }
    if (!pieceTextures[B_BISHOP - 1].loadFromFile("images/black-bishop.png")) {
        cerr << "Failed to load black bishop texture!" << endl;
        return false;
    }
    if (!pieceTextures[B_KNIGHT - 1].loadFromFile("images/black-knight.png")) {
        cerr << "Failed to load black knight texture!" << endl;
        return false;
    }
    if (!pieceTextures[B_ROOK - 1].loadFromFile("images/black-rook.png")) {
        cerr << "Failed to load black rook texture!" << endl;
        return false;
    }
    if (!pieceTextures[B_PAWN - 1].loadFromFile("images/black-pawn.png")) {
        cerr << "Failed to load black pawn texture!" << endl;
        return false;
    }

    return true;
}

void initializeBoard() {
    // Clear board
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            board[r][c] = NONE;
        }
    }

    // Set up initial board position
    // Black pieces (top)
    board[0][0] = B_ROOK; board[0][1] = B_KNIGHT; board[0][2] = B_BISHOP; board[0][3] = B_QUEEN;
    board[0][4] = B_KING; board[0][5] = B_BISHOP; board[0][6] = B_KNIGHT; board[0][7] = B_ROOK;

    // White pieces (bottom)
    board[7][0] = W_ROOK; board[7][1] = W_KNIGHT; board[7][2] = W_BISHOP; board[7][3] = W_QUEEN;
    board[7][4] = W_KING; board[7][5] = W_BISHOP; board[7][6] = W_KNIGHT; board[7][7] = W_ROOK;

    // Pawns
    for (int c = 0; c < 8; c++) {
        board[1][c] = B_PAWN;
        board[6][c] = W_PAWN;
    }
}

void initializeSprites() {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (board[r][c] != NONE) {
                int textureIndex = board[r][c] - 1;
                pieceSprites[r][c].setTexture(pieceTextures[textureIndex]);

                // Calculate scale to fit within square with some padding
                float textureWidth = pieceTextures[textureIndex].getSize().x;
                float textureHeight = pieceTextures[textureIndex].getSize().y;
                float scale = (squareSize * 0.8f) / std::max(textureWidth, textureHeight);

                pieceSprites[r][c].setScale(scale, scale);

                // Center the sprite in the square
                FloatRect bounds = pieceSprites[r][c].getLocalBounds();
                pieceSprites[r][c].setOrigin(bounds.width / 2, bounds.height / 2);
                updateSpritePosition(r, c);
            }
        }
    }
}

void updateSpritePosition(int row, int col) {
    pieceSprites[row][col].setPosition(
        col * squareSize + LABEL_MARGIN + squareSize / 2,
        row * squareSize + LABEL_MARGIN + squareSize / 2
    );
}

void handleMouseClick(const Event& event, const RenderWindow& window) {
    if (event.mouseButton.button == Mouse::Left) {
        Vector2f worldPos = window.mapPixelToCoords(Vector2i(event.mouseButton.x, event.mouseButton.y));
        int scol = int((worldPos.x - LABEL_MARGIN) / squareSize);
        int srow = int((worldPos.y - LABEL_MARGIN) / squareSize);

        if (scol >= 0 && scol < boardSize && srow >= 0 && srow < boardSize) {
            // Handle pawn promotion FIRST
            if (pawnPromotionPending) {
                // Auto-promote to queen immediately when any square is clicked
                promotePawn(promotionPawnType == W_PAWN ? W_QUEEN : B_QUEEN);
                pawnPromotionPending = false;

                // Check for game end conditions after promotion
                updateKingPosition();
                whiteInCheck = isInCheck(true);
                blackInCheck = isInCheck(false);

                if (isCheckmate(!whiteTurn)) {
                    gameOver = true;
                    gameResult = (whiteTurn ? "White" : "Black") + string(" wins by checkmate!");
                    playSoundEffect(gameEndSound);
                    cout << "CHECKMATE DETECTED AFTER PROMOTION!" << endl;
                }
                else if (isStalemate(!whiteTurn)) {
                    gameOver = true;
                    gameResult = "Stalemate!";
                    playSoundEffect(gameEndSound);
                }
                else {
                    whiteTurn = !whiteTurn; // Switch turn only if game not over
                }

                clearHighlights();
                selectedRow = -1;
                selectedCol = -1;
                return;
            }

            // If clicking a highlighted move
            if (selectedRow != -1 && highlightMovesArr[srow][scol]) {
                // Check if this is a castling move
                int movingPiece = board[selectedRow][selectedCol];
                bool isCapture = board[srow][scol] != NONE;

                if (movingPiece == W_KING || movingPiece == B_KING) {
                    if (abs(selectedCol - scol) == 2) {
                        // This is a castling move
                        performCastling(scol > selectedCol);
                        playSoundEffect(castleSound);
                        whiteTurn = !whiteTurn;

                        // Check for game end conditions
                        updateKingPosition();
                        whiteInCheck = isInCheck(true);
                        blackInCheck = isInCheck(false);

                        if (isCheckmate(!whiteTurn)) {
                            gameOver = true;
                            gameResult = (whiteTurn ? "White" : "Black") + string(" wins by checkmate!");
                            playSoundEffect(gameEndSound);
                            cout << "CHECKMATE DETECTED AFTER CASTLING!" << endl;
                        }

                        clearHighlights();
                        selectedRow = -1;
                        selectedCol = -1;
                        return;
                    }
                }

                // Store move in history before executing
                if (moveHistoryCount < MAX_MOVES) {
                    moveHistory_fromRow[moveHistoryCount] = selectedRow;
                    moveHistory_fromCol[moveHistoryCount] = selectedCol;
                    moveHistory_toRow[moveHistoryCount] = srow;
                    moveHistory_toCol[moveHistoryCount] = scol;
                    moveHistory_pieceMoved[moveHistoryCount] = board[selectedRow][selectedCol];
                    moveHistory_pieceCaptured[moveHistoryCount] = board[srow][scol];
                    moveHistoryCount++;
                }

                // Play appropriate sound
                if (isCapture) {
                    playSoundEffect(captureSound);
                }
                else {
                    playSoundEffect(moveSound);
                }

                // Execute the move
                movePiece(selectedRow, selectedCol, srow, scol);

                // Check for game end conditions immediately after move
                updateKingPosition();
                whiteInCheck = isInCheck(true);
                blackInCheck = isInCheck(false);

                // Play check sound if applicable
                if (whiteInCheck || blackInCheck) {
                    playSoundEffect(checkSound);
                }

                // Check for checkmate on the player who just got moved against
                if (isCheckmate(!whiteTurn)) {
                    gameOver = true;
                    gameResult = (whiteTurn ? "White" : "Black") + string(" wins by checkmate!");
                    playSoundEffect(gameEndSound);
                    cout << "CHECKMATE DETECTED!" << endl;
                }
                else if (isStalemate(!whiteTurn)) {
                    gameOver = true;
                    gameResult = "Stalemate!";
                    playSoundEffect(gameEndSound);
                }
                else {
                    whiteTurn = !whiteTurn; // Only switch turn if game is not over
                }

                clearHighlights();
                selectedRow = -1;
                selectedCol = -1;
                selectedPieceType = NONE;
                return;
            }

            // Select piece
            selectedRow = srow;
            selectedCol = scol;
            selectedPieceType = board[srow][scol];

            // Turn validation
            if (selectedPieceType != NONE) {
                bool isWhitePiece = isWhite(selectedPieceType);
                if ((whiteTurn && !isWhitePiece) || (!whiteTurn && isWhitePiece)) {
                    playSoundEffect(illegalMoveSound);
                    selectedRow = selectedCol = -1;
                    selectedPieceType = NONE;
                    clearHighlights();
                    return;
                }
            }

            clearHighlights();
            if (selectedPieceType != NONE) {
                generateValidMoves(srow, scol);
            }
        }
    }
}

void promotePawn(int promotionType) {
    if (!pawnPromotionPending) return;

    cout << "Promoting pawn at " << promotionRow << "," << promotionCol << " to " << promotionType << endl;

    board[promotionRow][promotionCol] = promotionType;

    // Update sprite
    int textureIndex = promotionType - 1;
    pieceSprites[promotionRow][promotionCol].setTexture(pieceTextures[textureIndex]);

    // Recalculate scale for the new piece
    float textureWidth = pieceTextures[textureIndex].getSize().x;
    float textureHeight = pieceTextures[textureIndex].getSize().y;
    float scale = (squareSize * 0.8f) / std::max(textureWidth, textureHeight);

    pieceSprites[promotionRow][promotionCol].setScale(scale, scale);

    // Re-center the sprite
    FloatRect bounds = pieceSprites[promotionRow][promotionCol].getLocalBounds();
    pieceSprites[promotionRow][promotionCol].setOrigin(bounds.width / 2, bounds.height / 2);
    updateSpritePosition(promotionRow, promotionCol);

    // Play promotion sound
    playSoundEffect(promoteSound);

    pawnPromotionPending = false;
    cout << "Pawn promotion completed!" << endl;
}

// ========== CHESS LOGIC FUNCTIONS ==========

bool isSquareAttacked(int row, int col, bool byWhite) {
    // Check for pawn attacks
    int pawnDir = byWhite ? -1 : 1;
    if (insideBoard(row + pawnDir, col - 1) &&
        board[row + pawnDir][col - 1] == (byWhite ? W_PAWN : B_PAWN)) return true;
    if (insideBoard(row + pawnDir, col + 1) &&
        board[row + pawnDir][col + 1] == (byWhite ? W_PAWN : B_PAWN)) return true;

    // Check for knight attacks
    int knightMoves[8][2] = { {-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1} };
    for (auto& move : knightMoves) {
        int r = row + move[0], c = col + move[1];
        if (insideBoard(r, c) && board[r][c] == (byWhite ? W_KNIGHT : B_KNIGHT)) return true;
    }

    // Check for king attacks (adjacent squares)
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int r = row + dr, c = col + dc;
            if (insideBoard(r, c) && board[r][c] == (byWhite ? W_KING : B_KING)) return true;
        }
    }

    // Check for rook/queen attacks (horizontal/vertical)
    int rookDirs[4][2] = { {-1,0},{1,0},{0,-1},{0,1} };
    for (auto& dir : rookDirs) {
        int r = row + dir[0], c = col + dir[1];
        while (insideBoard(r, c)) {
            if (board[r][c] != NONE) {
                if (board[r][c] == (byWhite ? W_ROOK : B_ROOK) ||
                    board[r][c] == (byWhite ? W_QUEEN : B_QUEEN)) return true;
                break;
            }
            r += dir[0]; c += dir[1];
        }
    }

    // Check for bishop/queen attacks (diagonal)
    int bishopDirs[4][2] = { {-1,-1},{-1,1},{1,-1},{1,1} };
    for (auto& dir : bishopDirs) {
        int r = row + dir[0], c = col + dir[1];
        while (insideBoard(r, c)) {
            if (board[r][c] != NONE) {
                if (board[r][c] == (byWhite ? W_BISHOP : B_BISHOP) ||
                    board[r][c] == (byWhite ? W_QUEEN : B_QUEEN)) return true;
                break;
            }
            r += dir[0]; c += dir[1];
        }
    }

    return false;
}

bool isInCheck(bool whiteKing) {
    if (whiteKing) {
        return isSquareAttacked(whiteKingRow, whiteKingCol, false);
    }
    else {
        return isSquareAttacked(blackKingRow, blackKingCol, true);
    }
}


void updateKingPosition() {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (board[r][c] == W_KING) {
                whiteKingRow = r; whiteKingCol = c;
            }
            else if (board[r][c] == B_KING) {
                blackKingRow = r; blackKingCol = c;
            }
        }
    }
}

void generateValidMoves(int r, int c) {
    int piece = board[r][c];
    if (piece == NONE) return;

    // Store original board state
    int tempBoard[8][8];
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            tempBoard[i][j] = board[i][j];
        }
    }

    // Generate pseudo-legal moves
    switch (piece) {
    case W_ROOK: case B_ROOK: genRookMoves(r, c); break;
    case W_BISHOP: case B_BISHOP: genBishopMoves(r, c); break;
    case W_KNIGHT: case B_KNIGHT: genKnightMoves(r, c); break;
    case W_KING: case B_KING: genKingMoves(r, c); break;
    case W_PAWN: case B_PAWN: genPawnMoves(r, c); break;
    case W_QUEEN: case B_QUEEN: genQueenMoves(r, c); break;
    default: break;
    }

    // Filter out moves that would leave king in check
    for (int tr = 0; tr < 8; tr++) {
        for (int tc = 0; tc < 8; tc++) {
            if (highlightMovesArr[tr][tc]) {
                // Simulate move
                int captured = board[tr][tc];
                board[tr][tc] = piece;
                board[r][c] = NONE;

                // Update king position if moving king - FIXED: Initialize variables
                int oldWhiteKingRow = whiteKingRow, oldWhiteKingCol = whiteKingCol;
                int oldBlackKingRow = blackKingRow, oldBlackKingCol = blackKingCol;

                if (piece == W_KING) {
                    whiteKingRow = tr; whiteKingCol = tc;
                }
                else if (piece == B_KING) {
                    blackKingRow = tr; blackKingCol = tc;
                }

                // Check if move leaves king in check
                bool leavesInCheck = isInCheck(isWhite(piece));

                // Restore king position
                if (piece == W_KING) {
                    whiteKingRow = oldWhiteKingRow; whiteKingCol = oldWhiteKingCol;
                }
                else if (piece == B_KING) {
                    blackKingRow = oldBlackKingRow; blackKingCol = oldBlackKingCol;
                }

                // Restore board
                board[r][c] = piece;
                board[tr][tc] = captured;

                // Remove invalid move
                if (leavesInCheck) {
                    highlightMovesArr[tr][tc] = false;
                }
            }
        }
    }
}

void genRookMoves(int r, int c) {
    int me = board[r][c];
    const int dr[4] = { -1, 1, 0, 0 };
    const int dc[4] = { 0, 0, -1, 1 };

    for (int d = 0; d < 4; ++d) {
        int rr = r + dr[d];
        int cc = c + dc[d];
        while (insideBoard(rr, cc)) {
            if (board[rr][cc] == NONE) {
                highlightMovesArr[rr][cc] = true;
            }
            else {
                if (isEnemyPiece(board[rr][cc], me)) {
                    highlightMovesArr[rr][cc] = true;
                }
                break;
            }
            rr += dr[d]; cc += dc[d];
        }
    }
}

void genBishopMoves(int r, int c) {
    int me = board[r][c];
    const int dr[4] = { -1, -1, 1, 1 };
    const int dc[4] = { -1, 1, -1, 1 };

    for (int d = 0; d < 4; ++d) {
        int rr = r + dr[d];
        int cc = c + dc[d];
        while (insideBoard(rr, cc)) {
            if (board[rr][cc] == NONE) {
                highlightMovesArr[rr][cc] = true;
            }
            else {
                if (isEnemyPiece(board[rr][cc], me)) {
                    highlightMovesArr[rr][cc] = true;
                }
                break;
            }
            rr += dr[d]; cc += dc[d];
        }
    }
}

void genKnightMoves(int r, int c) {
    int me = board[r][c];
    const int dr[8] = { -2,-1,1,2,2,1,-1,-2 };
    const int dc[8] = { 1,2,2,1,-1,-2,-2,-1 };

    for (int i = 0; i < 8; ++i) {
        int rr = r + dr[i], cc = c + dc[i];
        if (!insideBoard(rr, cc)) continue;
        if (board[rr][cc] == NONE || isEnemyPiece(board[rr][cc], me)) {
            highlightMovesArr[rr][cc] = true;
        }
    }
}

void genKingMoves(int r, int c) {
    int me = board[r][c];

    // Normal king moves
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int rr = r + dr, cc = c + dc;
            if (!insideBoard(rr, cc)) continue;
            if (board[rr][cc] == NONE || isEnemyPiece(board[rr][cc], me)) {
                highlightMovesArr[rr][cc] = true;
            }
        }
    }

    // Castling - White
    if (me == W_KING && r == 7 && c == 4 && !whiteKingMoved && !whiteInCheck) {
        // Kingside castling
        if (!whiteRookKingMoved &&
            board[7][5] == NONE && board[7][6] == NONE &&
            !isSquareAttacked(7, 5, false) && !isSquareAttacked(7, 6, false)) {
            highlightMovesArr[7][6] = true;
            cout << "White kingside castling available" << endl;
        }
        // Queenside castling
        if (!whiteRookQueenMoved &&
            board[7][3] == NONE && board[7][2] == NONE && board[7][1] == NONE &&
            !isSquareAttacked(7, 3, false) && !isSquareAttacked(7, 2, false)) {
            highlightMovesArr[7][2] = true;
            cout << "White queenside castling available" << endl;
        }
    }

    // Castling - Black
    if (me == B_KING && r == 0 && c == 4 && !blackKingMoved && !blackInCheck) {
        // Kingside castling
        if (!blackRookKingMoved &&
            board[0][5] == NONE && board[0][6] == NONE &&
            !isSquareAttacked(0, 5, true) && !isSquareAttacked(0, 6, true)) {
            highlightMovesArr[0][6] = true;
            cout << "Black kingside castling available" << endl;
        }
        // Queenside castling
        if (!blackRookQueenMoved &&
            board[0][3] == NONE && board[0][2] == NONE && board[0][1] == NONE &&
            !isSquareAttacked(0, 3, true) && !isSquareAttacked(0, 2, true)) {
            highlightMovesArr[0][2] = true;
            cout << "Black queenside castling available" << endl;
        }
    }
}

void genPawnMoves(int r, int c) {
    int me = board[r][c];

    if (me == W_PAWN) {
        // Forward move
        if (insideBoard(r - 1, c) && board[r - 1][c] == NONE) {
            highlightMovesArr[r - 1][c] = true;
            // Double move from starting position
            if (r == 6 && board[4][c] == NONE) {
                highlightMovesArr[4][c] = true;
            }
        }

        // Captures
        if (insideBoard(r - 1, c - 1) && isBlack(board[r - 1][c - 1])) {
            highlightMovesArr[r - 1][c - 1] = true;
        }
        if (insideBoard(r - 1, c + 1) && isBlack(board[r - 1][c + 1])) {
            highlightMovesArr[r - 1][c + 1] = true;
        }

        // En Passant
        if (r == 3 && enPassantPossible && enPassantTargetRow == 2) {
            if (c - 1 == enPassantTargetCol) highlightMovesArr[2][c - 1] = true;
            if (c + 1 == enPassantTargetCol) highlightMovesArr[2][c + 1] = true;
        }

    }
    else if (me == B_PAWN) {
        // Forward move
        if (insideBoard(r + 1, c) && board[r + 1][c] == NONE) {
            highlightMovesArr[r + 1][c] = true;
            // Double move from starting position
            if (r == 1 && board[3][c] == NONE) {
                highlightMovesArr[3][c] = true;
            }
        }

        // Captures
        if (insideBoard(r + 1, c - 1) && isWhite(board[r + 1][c - 1])) {
            highlightMovesArr[r + 1][c - 1] = true;
        }
        if (insideBoard(r + 1, c + 1) && isWhite(board[r + 1][c + 1])) {
            highlightMovesArr[r + 1][c + 1] = true;
        }

        // En Passant
        if (r == 4 && enPassantPossible && enPassantTargetRow == 5) {
            if (c - 1 == enPassantTargetCol) highlightMovesArr[5][c - 1] = true;
            if (c + 1 == enPassantTargetCol) highlightMovesArr[5][c + 1] = true;
        }
    }
}

void genQueenMoves(int r, int c) {
    genRookMoves(r, c);
    genBishopMoves(r, c);
}

void movePiece(int sr, int sc, int tr, int tc) {
    int movingPiece = board[sr][sc];

    // Handle en passant capture
    if ((movingPiece == W_PAWN || movingPiece == B_PAWN) && sc != tc && board[tr][tc] == NONE) {
        // This is an en passant capture
        if (movingPiece == W_PAWN) {
            board[tr + 1][tc] = NONE;
            // Hide the captured pawn sprite
            pieceSprites[tr + 1][tc].setPosition(-1000, -1000);
        }
        else {
            board[tr - 1][tc] = NONE;
            pieceSprites[tr - 1][tc].setPosition(-1000, -1000);
        }
    }

    // Update castling flags (for non-castling moves)
    if (movingPiece == W_KING) whiteKingMoved = true;
    if (movingPiece == B_KING) blackKingMoved = true;
    if (movingPiece == W_ROOK) {
        if (sr == 7 && sc == 0) whiteRookQueenMoved = true;
        if (sr == 7 && sc == 7) whiteRookKingMoved = true;
    }
    if (movingPiece == B_ROOK) {
        if (sr == 0 && sc == 0) blackRookQueenMoved = true;
        if (sr == 0 && sc == 7) blackRookKingMoved = true;
    }

    // Set en passant target for next move
    enPassantPossible = false;
    if ((movingPiece == W_PAWN || movingPiece == B_PAWN) && abs(sr - tr) == 2) {
        enPassantPossible = true;
        enPassantTargetRow = (sr + tr) / 2;
        enPassantTargetCol = sc;
    }

    // Handle pawn promotion
    if ((movingPiece == W_PAWN && tr == 0) || (movingPiece == B_PAWN && tr == 7)) {
        pawnPromotionPending = true;
        promotionRow = tr;
        promotionCol = tc;
        promotionPawnType = movingPiece;
        cout << "Pawn promotion triggered at " << tr << "," << tc << endl;
    }

    // Move the sprite
    pieceSprites[tr][tc] = pieceSprites[sr][sc];
    updateSpritePosition(tr, tc);

    // Hide the old sprite
    pieceSprites[sr][sc].setPosition(-1000, -1000);

    // Update board
    board[tr][tc] = movingPiece;
    board[sr][sc] = NONE;

    // Update king position if king moved
    if (movingPiece == W_KING) {
        whiteKingRow = tr;
        whiteKingCol = tc;
    }
    else if (movingPiece == B_KING) {
        blackKingRow = tr;
        blackKingCol = tc;
    }
}

void performCastling(bool kingside) {
    if (whiteTurn) {
        if (kingside) {
            // White kingside castling
            cout << "Performing white kingside castling" << endl;

            // Move king
            board[7][6] = W_KING;
            board[7][4] = NONE;
            pieceSprites[7][6] = pieceSprites[7][4];
            updateSpritePosition(7, 6);
            pieceSprites[7][4].setPosition(-1000, -1000);

            // Move rook
            board[7][5] = W_ROOK;
            board[7][7] = NONE;
            pieceSprites[7][5] = pieceSprites[7][7];
            updateSpritePosition(7, 5);
            pieceSprites[7][7].setPosition(-1000, -1000);

            // Update king position
            whiteKingRow = 7;
            whiteKingCol = 6;
        }
        else {
            // White queenside castling
            cout << "Performing white queenside castling" << endl;

            // Move king
            board[7][2] = W_KING;
            board[7][4] = NONE;
            pieceSprites[7][2] = pieceSprites[7][4];
            updateSpritePosition(7, 2);
            pieceSprites[7][4].setPosition(-1000, -1000);

            // Move rook
            board[7][3] = W_ROOK;
            board[7][0] = NONE;
            pieceSprites[7][3] = pieceSprites[7][0];
            updateSpritePosition(7, 3);
            pieceSprites[7][0].setPosition(-1000, -1000);

            // Update king position
            whiteKingRow = 7;
            whiteKingCol = 2;
        }
        whiteKingMoved = true;
    }
    else {
        // Black castling
        if (kingside) {
            // Black kingside castling
            cout << "Performing black kingside castling" << endl;

            // Move king
            board[0][6] = B_KING;
            board[0][4] = NONE;
            pieceSprites[0][6] = pieceSprites[0][4];
            updateSpritePosition(0, 6);
            pieceSprites[0][4].setPosition(-1000, -1000);

            // Move rook
            board[0][5] = B_ROOK;
            board[0][7] = NONE;
            pieceSprites[0][5] = pieceSprites[0][7];
            updateSpritePosition(0, 5);
            pieceSprites[0][7].setPosition(-1000, -1000);

            // Update king position
            blackKingRow = 0;
            blackKingCol = 6;
        }
        else {
            // Black queenside castling
            cout << "Performing black queenside castling" << endl;

            // Move king
            board[0][2] = B_KING;
            board[0][4] = NONE;
            pieceSprites[0][2] = pieceSprites[0][4];
            updateSpritePosition(0, 2);
            pieceSprites[0][4].setPosition(-1000, -1000);

            // Move rook
            board[0][3] = B_ROOK;
            board[0][0] = NONE;
            pieceSprites[0][3] = pieceSprites[0][0];
            updateSpritePosition(0, 3);
            pieceSprites[0][0].setPosition(-1000, -1000);

            // Update king position
            blackKingRow = 0;
            blackKingCol = 2;
        }
        blackKingMoved = true;
    }
}

bool hasValidMoves(bool forWhite) {
    // Check if any piece of the given color has any valid moves
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int piece = board[r][c];
            if (piece != NONE && ((forWhite && isWhite(piece)) || (!forWhite && isBlack(piece)))) {
                // Generate moves for this piece
                clearHighlights();
                generateValidMoves(r, c);

                // Check if any valid moves exist
                for (int tr = 0; tr < 8; tr++) {
                    for (int tc = 0; tc < 8; tc++) {
                        if (highlightMovesArr[tr][tc]) {
                            clearHighlights();
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

bool isCheckmate(bool forWhite) {
    bool inCheck = isInCheck(forWhite);
    bool hasMoves = hasValidMoves(forWhite);
    cout << "Checkmate check: forWhite=" << forWhite << " inCheck=" << inCheck << " hasMoves=" << hasMoves << endl;
    return inCheck && !hasMoves;
}

bool isStalemate(bool forWhite) {
    return !isInCheck(forWhite) && !hasValidMoves(forWhite);
}