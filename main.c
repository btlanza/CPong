/*
   CPong
   A simple two-player pong styled game written in SFML with the
   C language. Features a nice collision detection solution.

   Benjamin Lanza
   9/20/2018
*/

/* Standard C includes */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <time.h>

/* SFML includes */
#include <SFML/Audio.h>
#include <SFML/Graphics.h>

/* Window property definitions */
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define WINDOW_COLOR_DEPTH 32

/* Game object starting position definitions */
#define P1_START_X 75.0f
#define P1_START_Y 250.0f
#define P2_START_X 700.0f
#define P2_START_Y 250.0f
#define BALL_START_X 390.0f
#define BALL_START_Y 290.0f

/* Game object size and speed definitions */
#define BALL_SIZE 20.0f
#define PADDLE_WIDTH 25.0f
#define PADDLE_HEIGHT 100.0f
#define BALL_SPEED 5.0f
#define PADDLE_SPEED 8.0f

/* Min and max macros */
#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX(a, b) (((a)>(b))?(a):(b))

/* Side enum definition used for collision detection */
typedef enum {TOP, RIGHT, BOTTOM, LEFT} Side;

/* Declare 'Point' as an alias to the SFML sfVector2f type */
typedef sfVector2f Point;

/* Define a 'Line' struct as two points */
typedef struct Line{

	Point a;
	Point b;

} Line;

/* Define the 'Ball' struct */
typedef struct Ball{

	Point position;
	Point speed;

} Ball;

/* Define the 'Paddle' struct */
typedef struct Paddle{

	Point position;
	int score;

} Paddle;

/* Define the 'Collision' struct used to store collision detection return information */
typedef struct Collision{

	int collides;
	Point position;
	Side side;

} Collision;

/* Function declarations */
float Point_getDistance(Point a, Point b);
Point Ball_getVertex(const Ball *ball, int vertex);
Line Ball_getSide(const Ball *ball, Side side);
float Ball_getBound(const Ball *ball, Side side);
Point Paddle_getVertex(const Paddle *paddle, int vertex);
Line Paddle_getSide(const Paddle *paddle, Side side);
float Paddle_getBound(const Paddle *paddle, Side side);
Collision getPaddleCollision(const Ball *ball, const Paddle *paddle, Point *newPosition);
Collision getWallCollision(const Ball *ball, Point *newPosition);

/* Program entrypoint */
int main(int argc, char **argv){

	/* Engine setup */
	srand(time(NULL));
	size_t count;
	sfVideoMode mode = {WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_COLOR_DEPTH};
	sfRenderWindow *window;
	sfEvent event;

	/* Create the window */
	window = sfRenderWindow_create(mode, "CPong", sfClose, NULL);
	if(!window) return EXIT_FAILURE;
	sfRenderWindow_setVerticalSyncEnabled(window, sfTrue);
	sfRenderWindow_setFramerateLimit(window, 60);

	/* Game variable setup */
	int gameState = 0;
	int gameStarting = 0;
	time_t sinceStart = time(NULL);

	/* Define paddle and ball structs to keep track of position, points, and speed */
	Paddle p1 = {{P1_START_X, P1_START_Y}, 0};
	Paddle p2 = {{P2_START_X, P2_START_Y}, 0};
	Ball ball = {{BALL_START_X, BALL_START_Y}, {BALL_SPEED, BALL_SPEED}};

	/* Create SFML rectangles to display the objects */
	sfRectangleShape *p1Rect = sfRectangleShape_create();
	sfRectangleShape *p2Rect = sfRectangleShape_create();
	sfRectangleShape *ballRect = sfRectangleShape_create();

	/* Set the default positions */
	sfRectangleShape_setPosition(p1Rect, p1.position);
	sfRectangleShape_setPosition(p2Rect, p2.position);
	sfRectangleShape_setPosition(ballRect, ball.position);

	/* Set the sizes */
	sfRectangleShape_setSize(p1Rect, (sfVector2f){PADDLE_WIDTH, PADDLE_HEIGHT});
	sfRectangleShape_setSize(p2Rect, (sfVector2f){PADDLE_WIDTH, PADDLE_HEIGHT});
	sfRectangleShape_setSize(ballRect, (sfVector2f){BALL_SIZE, BALL_SIZE});

	/* Set the colors */
	sfRectangleShape_setFillColor(p1Rect, sfRed);
	sfRectangleShape_setFillColor(p2Rect, sfBlue);
	sfRectangleShape_setFillColor(ballRect, sfGreen);

	/* Print prompt to console */
	fprintf(stdout, "Press enter to start the game!\n");

	/* Loop while the window is open */
	while(sfRenderWindow_isOpen(window)){

		while(sfRenderWindow_pollEvent(window, &event)){
			if(event.type == sfEvtClosed) sfRenderWindow_close(window);
		}

		/* Game logic */
		if(gameState == 0){			/* Startup state */
			
			if(gameStarting){

				if(time(NULL) - sinceStart >= 3){
			
					/* Set object positions and generate random ball speed and direction */
					ball.position = (sfVector2f){BALL_START_X, BALL_START_Y};
					ball.speed = (sfVector2f){BALL_SPEED, 3.0f};
					if(rand() % 2) ball.speed.x *= -1.0f;
					ball.speed.y *= ((float)rand() / (float)RAND_MAX) * ((rand() % 2) ? -1.0f : 1.0f) + 0.1f;
					p1.position = (sfVector2f){P1_START_X, P1_START_Y};
					p2.position = (sfVector2f){P2_START_X, P2_START_Y};
					sfRectangleShape_setPosition(p1Rect, p1.position);
					sfRectangleShape_setPosition(p2Rect, p2.position);
					p1.score = 0; p2.score = 0;
					gameStarting = 0;
					++gameState;

				}

			/* Wait for the user to press the return key to start the match */
			}else if(sfKeyboard_isKeyPressed(sfKeyReturn)){

				gameStarting = 1;
				sinceStart = time(NULL);
				fprintf(stdout, "Starting the game in 3 seconds...\n");

			}
		
		}else if(gameState == 1){		/* Round loop state */

			/* Stores direction of player paddles and ball for later collision detection */
			int p1Dir = 0, p2Dir = 0;
			int ballXDir = (ball.speed.x >= 0) ? 1 : 0;
			int ballYDir = (ball.speed.y >= 0) ? 1 : 0;

			/* Check movement controls and update paddle positions */
			if(sfKeyboard_isKeyPressed(sfKeyW)){		/* W key pressed */
				float newY = p1.position.y - PADDLE_SPEED;
				newY = (newY > 0) ? newY : 0;
				p1.position.y = newY;
				sfRectangleShape_setPosition(p1Rect, p1.position);
				p1Dir -= 1;
			}

			if(sfKeyboard_isKeyPressed(sfKeyS)){		/* S key pressed */
				float newY = p1.position.y + PADDLE_SPEED;
				newY = (newY < WINDOW_HEIGHT - PADDLE_HEIGHT) ? newY : WINDOW_HEIGHT - PADDLE_HEIGHT;
				p1.position.y = newY;
				sfRectangleShape_setPosition(p1Rect, p1.position);	
				p1Dir += 1;	
			}
		
			if(sfKeyboard_isKeyPressed(sfKeyUp)){		/* Up key pressed */
				float newY = p2.position.y - PADDLE_SPEED;
				newY = (newY > 0) ? newY : 0;
				p2.position.y = newY;
				sfRectangleShape_setPosition(p2Rect, p2.position);
				p2Dir -= 1;	
			}

			if(sfKeyboard_isKeyPressed(sfKeyDown)){		/* Down key pressed */
				float newY = p2.position.y + PADDLE_SPEED;
				newY = (newY < WINDOW_HEIGHT - PADDLE_HEIGHT) ? newY : WINDOW_HEIGHT - PADDLE_HEIGHT;
				p2.position.y = newY;
				sfRectangleShape_setPosition(p2Rect, p2.position);
				p2Dir += 1;	
			}

			/* Get sfFloatRect objects for checking ball-paddle intersections */
			sfFloatRect p1FRect = sfRectangleShape_getGlobalBounds(p1Rect);
			sfFloatRect p2FRect = sfRectangleShape_getGlobalBounds(p2Rect);
			sfFloatRect ballFRect = sfRectangleShape_getGlobalBounds(ballRect);

			sfRectangleShape *pRect;
			sfFloatRect *pFRect;
			int *pDir;

			/* Check if the ball intersects a paddle after paddle movement */ 
			if(ballXDir == 1){ pRect = p2Rect; pFRect = &p2FRect; pDir = &p2Dir; }
			else{ pRect = p1Rect; pFRect = &p1FRect; pDir = &p1Dir; }

			if(sfFloatRect_intersects(pFRect, &ballFRect, NULL)){

				if(*pDir != 0){
					ball.speed.y = abs(ball.speed.y) * (float)*pDir;
					ball.speed.y += (*pDir == 1) ? PADDLE_SPEED : 0.0f - PADDLE_SPEED;
				}

			}
		
			/* Set up variables for line segment ball-paddle collision detection */
			Point ballNextA = {0.0f, 0.0f};
			Collision paddleCol;
			if(ball.speed.x >= 0) paddleCol = getPaddleCollision(&ball, &p2, &ballNextA);
			else paddleCol = getPaddleCollision(&ball, &p1, &ballNextA);

			/* Set up variables for line segment ball-wall collision detection */
			Point ballNextB = {0.0f, 0.0f};
			Collision wallCol = getWallCollision(&ball, &ballNextB);

			/* Check outcomes of ball-paddle collision tests and set ball speed and position accordingly */
			if(paddleCol.collides){
				ball.position = ballNextA;
				if(paddleCol.side == LEFT){
					ball.speed.x = ball.speed.x * -1.0f - 0.5f;
					ball.speed.y += (ballNextA.y + (0.5f * BALL_SIZE) - (p2.position.y + (0.5f * PADDLE_HEIGHT))) / 5.0f;
				}else if(paddleCol.side == RIGHT){
					ball.speed.x = ball.speed.x * -1.0f + 0.5f;
					ball.speed.y += (ballNextA.y + (0.5f * BALL_SIZE) - (p1.position.y + (0.5f * PADDLE_HEIGHT))) / 5.0f; 
				}
				else ball.speed.y *= -1.0f;
				if(ball.speed.y >= MAX(ball.speed.x * 3.0f, ball.speed.x * -3.0f))
					ball.speed.y = MAX(ball.speed.x * 3.0f, ball.speed.x * -3.0f);
				else if(ball.speed.y <= MIN(ball.speed.x * 3.0f, ball.speed.x * -3.0f))
					ball.speed.y = MIN(ball.speed.x * 3.0f, ball.speed.x * -3.0f);

			/* Check outcomes of ball-wall collision tests and set ball speed and position accordingly */
			}else if(wallCol.collides){
				if(wallCol.side == TOP || wallCol.side == BOTTOM){
					ball.speed.y *= -1.0f;
				}else if(wallCol.side == LEFT){
					++p2.score; ++gameState;
					fprintf(stdout, "Score is: %d to %d\n", p1.score, p2.score);
				}else if(wallCol.side == RIGHT){
					++p1.score; ++gameState;
					fprintf(stdout, "Score is: %d to %d\n", p1.score, p2.score);
				}
		
			/* If no collisions occured, update the balls position according to its unobstructed trajectory */
			}else{
				ball.position.x += ball.speed.x; ball.position.y += ball.speed.y;
			}

			/* Double check if the ball is out of bounds to handle an edge case */
			if(ball.position.y <= 0.0f) ball.position.y = 0.1f;
			else if(ball.position.y >= WINDOW_HEIGHT) ball.position.y = WINDOW_HEIGHT - BALL_SIZE - 0.1f;
			sfRectangleShape_setPosition(ballRect, ball.position);
			
		}else if(gameState == 2){			/* New round state */ 
			
			if(gameStarting){

				if(time(NULL) - sinceStart >= 1){
					
					/* Set object positions and generate random ball speed and direction */
					ball.position = (sfVector2f){BALL_START_X, BALL_START_Y};
					ball.speed = (sfVector2f){BALL_SPEED, 3.0f};
					if(rand() % 2) ball.speed.x *= -1.0f;
					ball.speed.y *= ((float)rand() / (float)RAND_MAX) * ((rand() % 2) ? -1.0f : 1.0f) + 0.1f;
					p1.position = (sfVector2f){P1_START_X, P1_START_Y};
					p2.position = (sfVector2f){P2_START_X, P2_START_Y};
					sfRectangleShape_setPosition(p1Rect, p1.position);
					sfRectangleShape_setPosition(p2Rect, p2.position);
					gameStarting = 0;
					--gameState;

				}

			}else if(p1.score >= 9 || p2.score >= 9){
				fprintf(stdout, "Player %d wins!\n", (MAX(p1.score, p2.score) == p1.score) ? 1 : 2);
				gameState = 0;
			}else{
				sinceStart = time(NULL);
				gameStarting = 1;
			}

		}


		/* Clear the screen */
		sfRenderWindow_clear(window, sfWhite);

		/* Draw objects to the buffer */	
		sfRenderWindow_drawRectangleShape(window, p1Rect, NULL);
		sfRenderWindow_drawRectangleShape(window, p2Rect, NULL);
		sfRenderWindow_drawRectangleShape(window, ballRect, NULL);

		/* Display the buffer */
		sfRenderWindow_display(window);

	}

	/* SFML object cleanup */
	sfRectangleShape_destroy(p1Rect);
	sfRectangleShape_destroy(p2Rect);
	sfRectangleShape_destroy(ballRect);
	sfRenderWindow_destroy(window);
	
	/* Exit successfully */
	return EXIT_SUCCESS;

}

/* Returns the distance between the two passed points */
float Point_getDistance(Point a, Point b){

	return sqrt(pow(b.x - a.x, 2) + (b.y - a.y, 2));

}

/* Gets the position of the vertex on the ball corresponding to the passed int identifier.
   The points are numbered as follows, from the origin (0) at the visual top-left position:

   0--1
   |  |
   3--2
   
   This clockwise point layout is used in the same way for all similar functions. */
Point Ball_getVertex(const Ball *ball, int vertex){

	switch(vertex){
		case 0:
			return ball->position;
		case 1:
			return (Point){ball->position.x + BALL_SIZE, ball->position.y};
		case 2:
			return (Point){ball->position.x + BALL_SIZE, ball->position.y + BALL_SIZE};
		case 3:
			return (Point){ball->position.x, ball->position.y + BALL_SIZE};
		default:
			return (Point){0.0f, 0.0f};
	}

}

/* Returns the line represented by the given side of the ball */
Line Ball_getSide(const Ball *ball, Side side){

	switch(side){
		case TOP:
			return (Line){Ball_getVertex(ball, 0), Ball_getVertex(ball, 1)};
		case RIGHT:
			return (Line){Ball_getVertex(ball, 1), Ball_getVertex(ball, 2)};
		case BOTTOM:
			return (Line){Ball_getVertex(ball, 2), Ball_getVertex(ball, 3)};
		case LEFT:
			return (Line){Ball_getVertex(ball, 3), Ball_getVertex(ball, 0)};
		default:
			return (Line){{0.0f, 0.0f}, {0.0f, 0.0f}};
	}

}

/* Returns the furthest extent of the given side of the ball.
   Naturally, TOP and BOTTOM will return a Y coordinate while LEFT and RIGHT will
   return an X coordinate. */
float Ball_getBound(const Ball *ball, Side side){

	switch(side){
		case TOP:
			return ball->position.y;
		case RIGHT:
			return ball->position.x + BALL_SIZE;
		case BOTTOM:
			return ball->position.y + BALL_SIZE;
		case LEFT:
			return ball->position.x;
		default:
			return 0.0f;
	}

}

/* Returns the position of the given vertex of the given paddle.
   See function 'Ball_getVertex' for more details. */
Point Paddle_getVertex(const Paddle *paddle, int vertex){

	switch(vertex){
		case 0:
			return paddle->position;
		case 1:
			return (Point){paddle->position.x + PADDLE_WIDTH, paddle->position.y};
		case 2:
			return (Point){paddle->position.x + PADDLE_WIDTH, paddle->position.y + PADDLE_HEIGHT};
		case 3:
			return (Point){paddle->position.x, paddle->position.y + PADDLE_HEIGHT};
		default:
			return (Point){0.0f, 0.0f};
	}

}

/* Returns the line representing the given side of the given paddle.
   See function 'Ball_getSide for more details. */
Line Paddle_getSide(const Paddle *paddle, Side side){

	switch(side){
		case TOP:
			return (Line){Paddle_getVertex(paddle, 0), Paddle_getVertex(paddle, 1)};
		case RIGHT:
			return (Line){Paddle_getVertex(paddle, 1), Paddle_getVertex(paddle, 2)};
		case BOTTOM:
			return (Line){Paddle_getVertex(paddle, 2), Paddle_getVertex(paddle, 3)};
		case LEFT:
			return (Line){Paddle_getVertex(paddle, 3), Paddle_getVertex(paddle, 0)};
		default:
			return (Line){{0.0f, 0.0f}, {0.0f, 0.0f}};
	}

}

/* Returns the extent of the paddle's bounds at the given side.
   See function 'Ball_getBound' for more details. */
float Paddle_getBound(const Paddle *paddle, Side side){

	switch(side){
		case TOP:
			return paddle->position.y;
		case RIGHT:
			return paddle->position.x + PADDLE_WIDTH;
		case BOTTOM:
			return paddle->position.y + PADDLE_HEIGHT;
		case LEFT:
			return paddle->position.x;
		default:
			return 0.0f;
	}

}

/* Tests collision between the ball and the passed paddle */
Collision getPaddleCollision(const Ball *ball, const Paddle *paddle, Point *newPosition){

	/* Declare the return value */
	Collision returnVal = (Collision){0, {0.0f, 0.0f}, TOP};

	/* Find the slope of the ball's path */
	float slopeN = ball->speed.y;
	float slopeD = ball->speed.x;

	/* Return failure if the slope is vertical */
	if(slopeD == 0.0f) return returnVal;
	float slope = slopeN / slopeD;

	/* Declare arrays to store collision test information */	
	int horizontalVertices[2];		/* Stores the vertices on the ball that collision will be tested against for horizontal collisions */
	int verticalVertices[2];		/* Stores the vertices on the ball that collision will be tested against for vertical collisions */
	int collisionVertices[2];		/* Stores the vertices on the ball where collisions actually occur */
	Side paddleSides[2];			/* Stores the names of paddle's sides where collisions will be tested against */
	Line paddleEdges[2];			/* Stores the points that define the paddle's sides where collision will be tested against */

	/* Determine the edges of the paddle that collision will be tested against */
	if(ball->speed.x >= 0.0f){
		
		if(Ball_getBound(ball, LEFT) > Paddle_getBound(paddle, RIGHT)) return returnVal;
		horizontalVertices[0] = 1; horizontalVertices[1] = 2;
		paddleSides[0] = LEFT;
		paddleEdges[0] = Paddle_getSide(paddle, LEFT);

	}else{

		if(Ball_getBound(ball, RIGHT) < Paddle_getBound(paddle, LEFT)) return returnVal;
		horizontalVertices[0] = 0; horizontalVertices[1] = 3;
		paddleSides[0] = RIGHT;
		paddleEdges[0] = Paddle_getSide(paddle, RIGHT);

	}

	if(ball->speed.y >= 0.0f){

		if(Ball_getBound(ball, TOP) > Paddle_getBound(paddle, BOTTOM)) return returnVal;	
		verticalVertices[0] = 2; verticalVertices[1] = 3;
		paddleSides[1] = TOP;
		paddleEdges[1] = Paddle_getSide(paddle, TOP);
	
	}else{

		if(Ball_getBound(ball, BOTTOM) < Paddle_getBound(paddle, TOP)) return returnVal;
		verticalVertices[0] = 0; verticalVertices[1] = 1;	
		paddleSides[1] = BOTTOM;
		paddleEdges[1] = Paddle_getSide(paddle, BOTTOM);

	}

	Collision horizontalCol = {0, {0.0f, 0.0f}, TOP};
	Collision verticalCol = {0, {0.0f, 0.0f}, TOP};

	/* Horizontal tests */
	for(int i = 0; i < 2; ++i){

		collisionVertices[0] = horizontalVertices[i];
			
		Point vertexPos = Ball_getVertex(ball, collisionVertices[0]);
		Point nextPos = {vertexPos.x + ball->speed.x, vertexPos.y + ball->speed.y};
			
		float ballMinX = MIN(vertexPos.x, nextPos.x), ballMaxX = MAX(vertexPos.x, nextPos.x);
		float paddleX = Paddle_getBound(paddle, paddleSides[0]);

		if(!(paddleX >= ballMinX && paddleX <= ballMaxX)) break;
	
		float yIncp = vertexPos.y - (vertexPos.x * slope);
		float paddleMinY = Paddle_getBound(paddle, TOP), paddleMaxY = Paddle_getBound(paddle, BOTTOM);
		float ballColY = (paddleX * slope) + yIncp;
			
		if(!(ballColY >= paddleMinY && ballColY <= paddleMaxY)) continue;
		else{
			horizontalCol.collides = 1;
			horizontalCol.position = (Point){paddleX, ballColY};
			horizontalCol.side = paddleSides[0];
			break;
		}

	}

	/* Vertical tests */
	for(int i = 0; i < 2; ++i){

		if(slope == 0.0f) break;

		collisionVertices[1] = verticalVertices[i];

		Point vertexPos = Ball_getVertex(ball, collisionVertices[1]);
		Point nextPos = {vertexPos.x + ball->speed.x, vertexPos.y + ball->speed.y};

		float ballMinY = MIN(vertexPos.y, nextPos.y), ballMaxY = MAX(vertexPos.y, nextPos.y);
		float paddleY = Paddle_getBound(paddle, paddleSides[1]);

		if(!(paddleY >= ballMinY && paddleY <= ballMaxY)) break;

		float yIncp = vertexPos.y - (vertexPos.x * slope);
		float paddleMinX = Paddle_getBound(paddle, LEFT), paddleMaxX = Paddle_getBound(paddle, RIGHT);
			float ballColX = (paddleY - yIncp) / slope;

		if(!(ballColX >= paddleMinX && ballColX <= paddleMaxX)) continue;
		else{
			verticalCol.collides = 1;
			verticalCol.position = (Point){ballColX, paddleY};
			verticalCol.side = paddleSides[1];
			break;
		}

	}

	/* Assigns the closest collision occurence to returnVal */
	int collisionVertex = 0;
	if(horizontalCol.collides && verticalCol.collides){
		int returnIndex = (Point_getDistance(Ball_getVertex(ball, collisionVertices[0]), horizontalCol.position) <
					Point_getDistance(Ball_getVertex(ball, collisionVertices[1]), verticalCol.position) ?
						0 : 1);
		collisionVertex = collisionVertices[returnIndex];
		returnVal = (returnIndex) ? verticalCol : horizontalCol;
	}
	else if(horizontalCol.collides){ returnVal = horizontalCol; collisionVertex = collisionVertices[0]; }
	else if(verticalCol.collides){ returnVal = verticalCol; collisionVertex = collisionVertices[1]; }
		
	if(returnVal.collides){

		Ball originReference = {returnVal.position, {0.0f, 0.0f}};
		Point backToOrigin = Ball_getVertex(&originReference, collisionVertex);
		newPosition->x = returnVal.position.x - (backToOrigin.x - returnVal.position.x);
		newPosition->y = returnVal.position.y - (backToOrigin.y - returnVal.position.y);
	}
	
	return returnVal;

}

/* Tests collision between the ball and the stage boundaries */
Collision getWallCollision(const Ball *ball, Point *newPosition){
	
	Collision returnVal = {0, {0.f, 0.f}, TOP};

	int testedVertex = -1;
	Side horizontalSide = -1, verticalSide = -1;
	float horizontalBound = -1.0f, verticalBound = -1.0f;
	
	if(ball->speed.x >= 0){
		horizontalSide = RIGHT;
		horizontalBound = WINDOW_WIDTH;
	}else{
		horizontalSide = LEFT; 
		horizontalBound = 0.0f;
	}

	if(ball->speed.y >= 0){
		verticalSide = BOTTOM;
		verticalBound = WINDOW_HEIGHT;
	}
	else{
		verticalSide = TOP;
		verticalBound = 0.0f;	
	}

	if(horizontalSide == RIGHT && verticalSide == BOTTOM) testedVertex = 2;
	else if(horizontalSide == LEFT && verticalSide == BOTTOM) testedVertex = 3;
	else if(horizontalSide == RIGHT && verticalSide == TOP) testedVertex = 1;
	else testedVertex = 0;

	float slopeN = ball->speed.y;
	float slopeD = ball->speed.x;

	if(slopeD == 0.0f) return returnVal;
	float slope = slopeN / slopeD;

	Point vertexPos = Ball_getVertex(ball, testedVertex);
	Point nextPos = {vertexPos.x + ball->speed.x, vertexPos.y + ball->speed.y};

	float yIncp = vertexPos.y - (vertexPos.x * slope);

	Collision horizontalCol = {0, {0.0f, 0.0f}, TOP};
	Collision verticalCol = {0, {0.0f, 0.0f}, TOP};

	if(MIN(vertexPos.x, nextPos.x) <= horizontalBound &&
		MAX(vertexPos.x, nextPos.x) >= horizontalBound){
		horizontalCol = (Collision){1, {horizontalBound, (slope * horizontalBound) + yIncp}, horizontalSide};
	}

	if(MIN(vertexPos.y, nextPos.y) <= verticalBound &&
		MAX(vertexPos.y, nextPos.y) >= verticalBound){
		verticalCol = (Collision){1, {(verticalBound - yIncp) / slope, verticalBound}, verticalSide};
	}


	if(horizontalCol.collides && verticalCol.collides){
		returnVal = (Point_getDistance(vertexPos, horizontalCol.position) <
				Point_getDistance(vertexPos, verticalCol.position) ?
					horizontalCol : verticalCol);
	}
	else if(horizontalCol.collides) returnVal = horizontalCol;
	else if(verticalCol.collides) returnVal = verticalCol;

	if(returnVal.collides){

		Ball originReference = {returnVal.position, {0.0f, 0.0f}};
		Point backToOrigin = Ball_getVertex(&originReference, testedVertex);
		newPosition->x = returnVal.position.x - (backToOrigin.x - returnVal.position.x);
		newPosition->y = returnVal.position.y - (backToOrigin.y - returnVal.position.y);
	}
	
	return returnVal;
}
