#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <unistd.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "SDL_gfxPrimitives.h"
#include "SDL_rotozoom.h"


#if defined(FUNKEY)

#define BUT_UP SDLK_u
#define BUT_RIGHT SDLK_r
#define BUT_DOWN SDLK_d
#define BUT_LEFT SDLK_l
#define BUT_A SDLK_a
#define BUT_X SDLK_x
#define BUT_Y SDLK_y
#define BUT_B SDLK_b
#define BUT_QUIT SDLK_q

#elif defined(MIYOO)

#define BUT_UP SDLK_UP
#define BUT_RIGHT SDLK_RIGHT
#define BUT_DOWN SDLK_DOWN
#define BUT_LEFT SDLK_LEFT
#define BUT_A SDLK_SPACE
#define BUT_X SDLK_LSHIFT
#define BUT_Y SDLK_LALT
#define BUT_B SDLK_LCTRL
#define BUT_QUIT SDLK_ESCAPE

#else

#define BUT_UP SDLK_UP
#define BUT_RIGHT SDLK_RIGHT
#define BUT_DOWN SDLK_DOWN
#define BUT_LEFT SDLK_LEFT
#define BUT_A SDLK_SPACE
#define BUT_X SDLK_x
#define BUT_Y SDLK_y
#define BUT_B SDLK_LCTRL
#define BUT_QUIT SDLK_ESCAPE


#endif

const int ORIG_WINDOW_WIDTH = 240;
const int ORIG_WINDOW_HEIGHT = 240;
int WINDOW_WIDTH = ORIG_WINDOW_WIDTH;
int WINDOW_HEIGHT = ORIG_WINDOW_HEIGHT;

enum GameStates {GSQuit,GSIntro,GSGame,GSGameOver};

const int FPS = 30;
SDL_Surface *Screen,*Background,*Player,*Enemy,*Buffer;
TTF_Font* font;
bool GlobalSoundEnabled = true;
GameStates GameState = GSIntro;
Uint32 NextTime;
bool EnemyStates[3][3];
bool PlayerStates[3];
bool TvOutMode = false;
int HitPosition, LivesLost;
Mix_Chunk *TickSound, *CrashSound;
long Score = 0, HiScore = 0;
SDL_Color FG = {0,0,0,0};
SDL_Color BG = {172,172,172,0};
Uint32 FrameTicks = 0;
Uint32 FrameCount = 0;
Uint32 LastFps = 0;
bool ShowFps = false;
bool noDelay = false;

void HandleFPS()
{
	if(!ShowFps)
		return;
	FrameCount++;
	char Text[100];
	sprintf(Text, "FPS:%d", LastFps);
    SDL_Color Col = {0, 0, 0, 255};
	SDL_Surface *Tmp = TTF_RenderText_Solid(font, Text, Col);
	boxRGBA(Screen,0,0,Tmp->w + 6, Tmp->h + 6,255,255,255,255);
	SDL_Rect Dst;
	Dst.x = 3;
	Dst.y = 3;
	Dst.w = Tmp->w;
	Dst.h = Tmp->h;
	SDL_BlitSurface(Tmp, NULL, Screen, &Dst);
	SDL_FreeSurface(Tmp);
	if(SDL_GetTicks() - FrameTicks >= 1000)
	{
		LastFps = FrameCount;
		FrameCount = 0;
		FrameTicks = SDL_GetTicks();
	}
}


Uint32 WaitForFrame()
{
	Uint32 Now,Result;
	Now = SDL_GetTicks();
	if (Now >= NextTime)
	{
		Result = 0;
		NextTime = Now + 1000/FPS;
	}
	else
		Result = NextTime - Now;
	return Result;
}

void LoadSettings()
{
 	FILE *SettingsFile;
	char Filename[4096];

	const char *File = ".com.github.joyrider377.formula1_funkey.dat";
	char *EnvHome = getenv("HOME");
	char *EnvHomeDrive = getenv("HOMEDRIVE");
	char *EnvHomePath = getenv("HOMEPATH");

	sprintf(Filename, "%s", File);
	if (EnvHome) //linux systems normally
		sprintf(Filename, "%s/%s", EnvHome, File);
	else
		if(EnvHomeDrive && EnvHomePath) //windows systems normally
			sprintf(Filename, "%s%s/%s", EnvHomeDrive, EnvHomePath, File);

 	SettingsFile = fopen(Filename,"rb");
 	if(SettingsFile)
 	{
		fread(&HiScore, sizeof(HiScore), 1, SettingsFile);
		fclose(SettingsFile);
	}

}

// Save the settings
void SaveSettings()
{
 	FILE *SettingsFile;
 	char Filename[4096];

	const char *File = ".com.github.joyrider377.formula1_funkey.dat";
	char *EnvHome = getenv("HOME");
	char *EnvHomeDrive = getenv("HOMEDRIVE");
	char *EnvHomePath = getenv("HOMEPATH");

	sprintf(Filename, "%s", File);
	if (EnvHome) //linux systems normally
		sprintf(Filename, "%s/%s", EnvHome, File);
	else
		if(EnvHomeDrive && EnvHomePath) //windows systems normally
			sprintf(Filename, "%s%s/%s", EnvHomeDrive, EnvHomePath, File);
 	
	SettingsFile = fopen(Filename,"wb");
	if(SettingsFile)
 	{
		fwrite(&HiScore, sizeof(HiScore), 1, SettingsFile);
		fflush(SettingsFile);
		fclose(SettingsFile);
 	}
}

void setHiScore(long value)
{
	if(value > HiScore)
	{
    	HiScore = value;
    	SaveSettings();
	}
}

void DrawScoreBar(bool Empty, long ScoreIn, long HighScoreIn, int LivesLostIn)
{
	if(Empty)
		return;

	char ScoreStr[20], Str[20];
	SDL_Surface *Text;
	SDL_Rect DstRect;
	sprintf(ScoreStr,"%ld", ScoreIn);
	if (strlen(ScoreStr) > 0)
	{
		Text = TTF_RenderText_Shaded(font,ScoreStr,FG,BG);
		DstRect.x = 200 - Text->w;
		DstRect.y = 30;
		DstRect.w = Text->w;
		DstRect.h = Text->h;
		SDL_BlitSurface(Text,NULL,Buffer,&DstRect);
		SDL_FreeSurface(Text);
	}
	sprintf(ScoreStr,"%ld", HighScoreIn);
	if (strlen(ScoreStr) > 0)
	{
		Text = TTF_RenderText_Shaded(font,ScoreStr,FG,BG);
		DstRect.x = 140 - Text->w;
		DstRect.y = 30;
		DstRect.w = Text->w;
		DstRect.h = Text->h;
		SDL_BlitSurface(Text,NULL,Buffer,&DstRect);
		SDL_FreeSurface(Text);
	}
	if (LivesLostIn >=1)
	{
		for(int X = 0;X<LivesLostIn;X++)
			Str[X] = 'X';
		Str[LivesLostIn] = '\0';
		Text =TTF_RenderText_Shaded(font,Str,FG,BG);
		DstRect.x = 50;
		DstRect.y = 30;
		DstRect.w = Text->w;
		DstRect.h = Text->h;
		SDL_BlitSurface(Text,NULL,Buffer,&DstRect);
		SDL_FreeSurface(Text);
	}
}

void DrawGame()
{
	int X,Y;
	SDL_Rect DstRect;
	for (X=0;X<3;X++)
		for (Y=0;Y<3;Y++)
			if (EnemyStates[X][Y])
			{
				DstRect.x = 50 + (X * 54);
				DstRect.y = 48 + (Y * 47);
				DstRect.w = Enemy->w;
				DstRect.h = Enemy->h;
				SDL_BlitSurface(Enemy,NULL,Buffer,&DstRect);
			}
	for (X=0;X<3;X++)
	{
		if (PlayerStates[X])
		{
			DstRect.x = 50 + (X * 54);
			DstRect.y = 180;
			DstRect.w = Player->w;
			DstRect.h = Player->h;
			SDL_BlitSurface(Player,NULL,Buffer,&DstRect);
		}
	}
}

void MoveEnemy()
{
	int X,Y;
	for (X=0;X<3;X++)
		for (Y=2;Y>=1;Y--)
			EnemyStates[X][Y] = EnemyStates[X][Y-1];
	for (X=0;X<3;X++)
		EnemyStates[X][0] = false;
	for (X=0;X<=1;X++)
		EnemyStates[rand()%3][0] = true;
}

void MoveLeft()
{
	int X;
	for (X=0;X<2;X++)
	{
		if (PlayerStates[X+1])
		{
			PlayerStates[X] = true;
			PlayerStates[X+1] = false;
		}
	}
}

void MoveRight()
{
	int X;
	for (X=2;X>=1;X--)
	{
		if (PlayerStates[X-1])
		{
			PlayerStates[X] = true;
			PlayerStates[X-1] = false;
		}
	}
}

bool IsCollided()
{
	int X;
	bool Temp;
	Temp=false;
	for (X = 0;X<3;X++)
		if (PlayerStates[X] && EnemyStates[X][2])
		{
			Temp = true;
			HitPosition = X;
		}
	return Temp;
}

void HitFlash()
{
	PlayerStates[HitPosition] = !PlayerStates[HitPosition];
	EnemyStates[HitPosition][2] = !EnemyStates[HitPosition][2];
}

void InitialiseStates()
{
	int X,Y;
	for(X = 0;X<3;X++)
		for(Y=0;Y<3;Y++)
			EnemyStates[X][Y] = false;
	for(X=0;X<3;X++)
		PlayerStates[X] = false;
	PlayerStates[1] = true;
}

void Game()
{
	SDL_Event Event;
	int Teller,FlashesDelay,Flashes,X,Delay;
	bool CanMove,CrashSoundPlayed;
	Teller = 25;
	FlashesDelay = 14;
	Flashes = 0;
	CanMove = true;
	Score = 0;
	Delay = 60;
	LivesLost = 0;
	CrashSoundPlayed = false;
	InitialiseStates();
	while (GameState == GSGame)
	{
		while (SDL_PollEvent(&Event))
		{
			if(Event.type == SDL_QUIT)
				GameState = GSQuit;

			if (Event.type == SDL_KEYDOWN)
			{
				switch(Event.key.keysym.sym)
				{
          			case BUT_QUIT:
						GameState= GSQuit;
						break;
					case BUT_LEFT:
						if(CanMove)
							MoveLeft();
						break;
					case BUT_RIGHT:
						if(CanMove)
							MoveRight();
						break;
					default:
						break;
				}
			}
		}
		SDL_BlitSurface(Background,NULL,Buffer,NULL);
		Teller++;
		if (Teller >= Delay)
		{
			if (!IsCollided() && CanMove)
			{
				Teller = 0;
				for (X = 0;X < 3;X++)
					if (EnemyStates[X][2])
					{
						Score += 10;
						setHiScore(Score);
						if ((Score % 100 ==0) && (Delay > 18))
							Delay--;
					}
				MoveEnemy();
				if (GlobalSoundEnabled)
					Mix_PlayChannel(-1,TickSound,0);
			}
			else
			{
				if (!CrashSoundPlayed)
				{
					if (GlobalSoundEnabled)
						Mix_PlayChannel(-1,CrashSound,0);
                    CrashSoundPlayed = true;
				}
				CanMove = false;
				FlashesDelay++;
				if (FlashesDelay == 15)
				{
					Flashes++;
					HitFlash();
					FlashesDelay = 0;
					if (Flashes == 6)
					{
						Flashes = 0;
						CanMove = true;
						Teller = 0;
						CrashSoundPlayed=false;
						EnemyStates[HitPosition][2] = false;
						LivesLost++;
						FlashesDelay = 14;
						if (LivesLost ==3)
							GameState = GSGameOver;
					}
				}
			}
		}
		DrawGame();
		DrawScoreBar(false, Score, HiScore, LivesLost);
        SDL_FillRect(Screen,NULL,SDL_MapRGB(Screen->format,0,0,0));
        if ((WINDOW_WIDTH != ORIG_WINDOW_WIDTH) || (WINDOW_HEIGHT != ORIG_WINDOW_HEIGHT))
		{
			double wscale = (double)WINDOW_WIDTH / ORIG_WINDOW_WIDTH;
			if(ORIG_WINDOW_HEIGHT * wscale > WINDOW_HEIGHT)
				wscale = (double)WINDOW_HEIGHT / ORIG_WINDOW_HEIGHT;
			SDL_Rect dst;
			dst.x = (WINDOW_WIDTH - (ORIG_WINDOW_WIDTH * wscale)) / 2;
			dst.y = (WINDOW_HEIGHT - (ORIG_WINDOW_HEIGHT * wscale)) / 2,
			dst.w = ORIG_WINDOW_WIDTH * wscale;
			dst.h = ORIG_WINDOW_HEIGHT * wscale;
			SDL_Surface *ScreenBufferZoom = zoomSurface(Buffer,wscale,wscale,0);
			SDL_BlitSurface(ScreenBufferZoom,NULL,Screen,&dst);
			SDL_FreeSurface(ScreenBufferZoom);
		}
		else
		{
			SDL_BlitSurface(Buffer, NULL, Screen, NULL);
		}
		HandleFPS();
		SDL_Flip(Screen);
		if(!noDelay)
			SDL_Delay(WaitForFrame());
	}

}

void GameOver()
{
	SDL_Event Event;
	while (GameState == GSGameOver)
	{
		while (SDL_PollEvent(&Event))
		{
			if(Event.type == SDL_QUIT)
				GameState = GSQuit;

			if (Event.type == SDL_KEYDOWN)
			{
				switch(Event.key.keysym.sym)
				{
          			case BUT_QUIT:
						GameState = GSQuit;
						break;
                    case BUT_LEFT:
                    case BUT_RIGHT:
                    case BUT_UP:
                    case BUT_DOWN:
					case BUT_A:
					case BUT_B:
					case BUT_X:
					case BUT_Y:
						GameState = GSGame;
						break;
					default:
						break;
				}
			}
		}
		SDL_BlitSurface(Background,NULL,Buffer,NULL);
		DrawGame();
		DrawScoreBar(false, Score, HiScore, LivesLost);
        SDL_FillRect(Screen,NULL,SDL_MapRGB(Screen->format,0,0,0));
        if ((WINDOW_WIDTH != ORIG_WINDOW_WIDTH) || (WINDOW_HEIGHT != ORIG_WINDOW_HEIGHT))
		{
			double wscale = (double)WINDOW_WIDTH / ORIG_WINDOW_WIDTH;
			if(ORIG_WINDOW_HEIGHT * wscale > WINDOW_HEIGHT)
				wscale = (double)WINDOW_HEIGHT / ORIG_WINDOW_HEIGHT;
			SDL_Rect dst;
			dst.x = (WINDOW_WIDTH - (ORIG_WINDOW_WIDTH * wscale)) / 2;
			dst.y = (WINDOW_HEIGHT - (ORIG_WINDOW_HEIGHT * wscale)) / 2,
			dst.w = ORIG_WINDOW_WIDTH * wscale;
			dst.h = ORIG_WINDOW_HEIGHT * wscale;
			SDL_Surface *ScreenBufferZoom = zoomSurface(Buffer,wscale,wscale,0);
			SDL_BlitSurface(ScreenBufferZoom,NULL,Screen,&dst);
			SDL_FreeSurface(ScreenBufferZoom);
		}
		else
		{
			SDL_BlitSurface(Buffer, NULL, Screen, NULL);
		}
		HandleFPS();
		SDL_Flip(Screen);
		if(!noDelay)
			SDL_Delay(WaitForFrame());
	}

}

void FlashIntro()
{
	int X,Y;
	for (X = 0;X < 3;X++)
 		for (Y = 0;Y<3;Y++)
 			EnemyStates[X][Y] = ! EnemyStates[X][Y];
 	for (X=0;X<3;X++)
 		PlayerStates[X] = ! PlayerStates[X];
}


void Intro()
{
	SDL_Event Event ;
	int FlashesDelay;
	FlashesDelay = 0;
	while (GameState == GSIntro)
	{
		while (SDL_PollEvent(&Event))
		{
			if(Event.type == SDL_QUIT)
				GameState = GSQuit;

			if (Event.type == SDL_KEYDOWN)
			{
				switch(Event.key.keysym.sym)
				{
					case BUT_QUIT:
						GameState = GSQuit;
						break;
					case BUT_LEFT:
					case BUT_RIGHT:
					case BUT_UP:
					case BUT_DOWN:
					case BUT_A:
					case BUT_B:
					case BUT_X:
					case BUT_Y:
						GameState = GSGame;
						break;
					default:
						break;
				}
			}
		}
		SDL_BlitSurface(Background,NULL,Buffer,NULL);
		FlashesDelay++;
		if (FlashesDelay == 25)
		{
			FlashesDelay = 0;
			FlashIntro();
		}
		DrawGame();
		DrawScoreBar(!PlayerStates[0], 888888, 888888, 3);
        SDL_FillRect(Screen,NULL,SDL_MapRGB(Screen->format,0,0,0));
        if ((WINDOW_WIDTH != ORIG_WINDOW_WIDTH) || (WINDOW_HEIGHT != ORIG_WINDOW_HEIGHT))
		{
			double wscale = (double)WINDOW_WIDTH / ORIG_WINDOW_WIDTH;
			if(ORIG_WINDOW_HEIGHT * wscale > WINDOW_HEIGHT)
				wscale = (double)WINDOW_HEIGHT / ORIG_WINDOW_HEIGHT;
			SDL_Rect dst;
			dst.x = (WINDOW_WIDTH - (ORIG_WINDOW_WIDTH * wscale)) / 2;
			dst.y = (WINDOW_HEIGHT - (ORIG_WINDOW_HEIGHT * wscale)) / 2,
			dst.w = ORIG_WINDOW_WIDTH * wscale;
			dst.h = ORIG_WINDOW_HEIGHT * wscale;
			SDL_Surface *ScreenBufferZoom = zoomSurface(Buffer,wscale,wscale,0);
			SDL_BlitSurface(ScreenBufferZoom,NULL,Screen,&dst);
			SDL_FreeSurface(ScreenBufferZoom);
		}
		else
		{
			SDL_BlitSurface(Buffer, NULL, Screen, NULL);
		}
		HandleFPS();
		
		SDL_Flip(Screen);
		if(!noDelay)
			SDL_Delay(WaitForFrame());
	}

}

int main(int argc, char **argv)
{
	bool useSoftwareRenderer = true;
    bool useFullScreenAtStartup = false;
	for (int i=0; i < argc; i++)
	{
		if(strcasecmp(argv[i], "-?") == 0)
		{
			printf("Usage: formula [Options]\n\n");
			printf("  Possible options are:\n");
			printf("    -?: show this help message\n");
			printf("    -a: Use Acclerated Renderer\n");
			printf("    -f: Run fullscreen at startup (by default starts up windowed)\n");
			printf("    -fps: Show FPS\n"); 
			printf("    -nd: No frame delay (run as fast as possible)\n");
			printf("    -s[x]: x = 1-5 scale window by this factor\n");
			exit(0);
		}
		if(strcasecmp(argv[i], "-a") == 0)
			useSoftwareRenderer = false;
		if(strcasecmp(argv[i], "-nd") == 0)
			noDelay = true;
		if(strcasecmp(argv[i], "-fps") == 0)
			ShowFps = true;
		if(strcasecmp(argv[i], "-f") == 0)
			useFullScreenAtStartup = true;
		if(strcasecmp(argv[i], "-s2") == 0)
		{
			WINDOW_WIDTH = ORIG_WINDOW_WIDTH * 2;
			WINDOW_HEIGHT = ORIG_WINDOW_HEIGHT * 2;
		}

		if(strcasecmp(argv[i], "-s3") == 0)
		{
			WINDOW_WIDTH = ORIG_WINDOW_WIDTH * 3;
			WINDOW_HEIGHT = ORIG_WINDOW_HEIGHT * 3;
		}

		if(strcasecmp(argv[i], "-s4") == 0)
		{
			WINDOW_WIDTH = ORIG_WINDOW_WIDTH * 4;
			WINDOW_HEIGHT = ORIG_WINDOW_HEIGHT * 4;
		}

		if(strcasecmp(argv[i], "-s5") == 0)
		{
			WINDOW_WIDTH = ORIG_WINDOW_WIDTH * 5;
			WINDOW_HEIGHT = ORIG_WINDOW_HEIGHT * 5;
		}
	}

#ifdef FUNKEY		
	useSoftwareRenderer = false;
	useFullScreenAtStartup = true;	
#endif

	
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO ) == 0)
	{
		printf("SDL Succesfully initialized\n");
		Uint32 Flags = 0;
        if (useFullScreenAtStartup) 
		{
			WINDOW_WIDTH = 0;
			WINDOW_HEIGHT = 0;
            Flags |= SDL_FULLSCREEN;
        }
		if (useSoftwareRenderer) 
			Flags |= SDL_SWSURFACE;
		else
			Flags |= SDL_HWSURFACE;
		Screen = SDL_SetVideoMode( WINDOW_WIDTH, WINDOW_HEIGHT,0, Flags );
		if(Screen)
		{
			WINDOW_WIDTH = Screen->w;
			WINDOW_HEIGHT = Screen->h;
		    SDL_Surface * Tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, ORIG_WINDOW_WIDTH, ORIG_WINDOW_HEIGHT,0,Screen->format->Rmask,Screen->format->Gmask,Screen->format->Bmask,Screen->format->Amask);
			Buffer = SDL_DisplayFormat(Tmp);
			SDL_FreeSurface(Tmp);
			printf("Succesfully Set %dx%dx32\n",WINDOW_WIDTH,WINDOW_HEIGHT);
			SDL_ShowCursor(SDL_DISABLE);
			if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0)
			{
				GlobalSoundEnabled = false;
				printf("Failed to initialise sound!\n");
			}
			else
			{
				printf("Audio Succesfully initialised!\n");
			}
			if (TTF_Init() == 0)
			{
				printf("Succesfully initialized TTF\n");
				font = TTF_OpenFont("./filesystem/font.ttf",14);
				if (font)
				{
					printf("Succesfully Loaded font\n");
					TTF_SetFontStyle(font,TTF_STYLE_NORMAL);
					srand(time(NULL));
					TickSound = Mix_LoadWAV("./filesystem/sounds/tick.wav");
					CrashSound = Mix_LoadWAV("./filesystem/sounds/crash.wav");
					Tmp = IMG_Load("./filesystem/graphics/background.png");
					Background = SDL_DisplayFormat(Tmp);
					SDL_FreeSurface(Tmp);
					Tmp = IMG_Load("./filesystem/graphics/player.png");
					Player = SDL_DisplayFormat(Tmp);
					SDL_FreeSurface(Tmp);
					Tmp = IMG_Load("./filesystem/graphics/enemy.png");
					Enemy = SDL_DisplayFormat(Tmp);
					SDL_FreeSurface(Tmp);
					SDL_ShowCursor(SDL_DISABLE);
					LoadSettings();
					while (GameState != GSQuit)
					{
						switch(GameState)
						{
							case GSIntro :
								Intro();
								break;
							case GSGame :
								Game();
								break;
							case GSGameOver:
								GameOver();
								break;
							default:
								break;
						}
					}
					SaveSettings();
					TTF_CloseFont(font);
					font=NULL;
				}
				else
				{
					printf("Failed to Load font\n");
				}
				TTF_Quit();
			}
			else
			{
				printf("Failed to initialize TTF\n");
			}
			Mix_FreeChunk(CrashSound);
			Mix_FreeChunk(TickSound);
			SDL_FreeSurface(Screen);
			SDL_FreeSurface(Background);
			SDL_FreeSurface(Player);
			SDL_FreeSurface(Enemy);
			Screen=NULL;
			Mix_CloseAudio();
		}
		else
		{
			printf("Failed to Set Videomode %dx%dx32\n",WINDOW_WIDTH, WINDOW_HEIGHT);
		}
		SDL_Quit();
	}
	else
	{
		printf("Couldn't initialise SDL!\n");
	}
	return 0;
}


