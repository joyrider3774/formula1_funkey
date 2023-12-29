#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <unistd.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_image.h"
#include "SDL_mixer.h"


const int WINDOW_WIDTH = 240;
const int WINDOW_HEIGHT = 240;

enum GameStates {GSQuit,GSIntro,GSGame,GSGameOver};

const int FPS = 30;
SDL_Surface *Screen,*Background,*Player,*Enemy,*Tmp,*Buffer;
TTF_Font* font;
bool GlobalSoundEnabled = true;
GameStates GameState = GSIntro;
Uint32 NextTime;
bool EnemyStates[3][3];
bool PlayerStates[3];
bool TvOutMode = false;
int HitPosition, LivesLost;
Mix_Chunk *TickSound, *CrashSound;
long Score = 0;
SDL_Color FG = {0,0,0,0};
SDL_Color BG = {172,172,172,0};

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

void DrawScoreBar(bool Empty)
{
	if(Empty)
		return;

	char ScoreStr[20], Str[20];
	SDL_Surface *Text;
	SDL_Rect DstRect;
	sprintf(ScoreStr,"%ld", Score);
	if (strlen(ScoreStr) > 0)
	{
		Text = TTF_RenderText_Shaded(font,ScoreStr,FG,BG);
		DstRect.x = 190 - Text->w;
		DstRect.y = 30;
		DstRect.w = Text->w;
		DstRect.h = Text->h;
		SDL_BlitSurface(Text,NULL,Buffer,&DstRect);
		SDL_FreeSurface(Text);
	}
	if (LivesLost >=1)
	{
		for(int X = 0;X<LivesLost;X++)
			Str[X] = 'X';
		Str[LivesLost] = '\0';
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
					case SDLK_ESCAPE:			
						GameState= GSQuit;
						break;
					case SDLK_LEFT:
						if(CanMove)
							MoveLeft();
						break;
					case SDLK_RIGHT:
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
						if ((Score % 100 ==0) & (Delay > 18))
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
		DrawScoreBar(false);
        SDL_FillRect(Screen,NULL,SDL_MapRGB(Screen->format,0,0,0));
        SDL_BlitSurface(Buffer,NULL,Screen,NULL);
		SDL_Flip(Screen);
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
					case SDLK_ESCAPE:
						GameState = GSQuit;
						break;
                    case SDLK_LEFT:
                    case SDLK_RIGHT:
                    case SDLK_UP:
                    case SDLK_DOWN:
					case SDLK_a:
					case SDLK_b:
					case SDLK_x:
					case SDLK_y:
						GameState = GSGame;
						break;
					default:
						break;
				}
			}
		}
		SDL_BlitSurface(Background,NULL,Buffer,NULL);
		DrawGame();
		DrawScoreBar(false);
        SDL_FillRect(Screen,NULL,SDL_MapRGB(Screen->format,0,0,0));
        SDL_BlitSurface(Buffer,NULL,Screen,NULL);
		SDL_Flip(Screen);
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
	LivesLost = 0;
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
					case SDLK_ESCAPE:
						GameState = GSQuit;
						break;
                   	case SDLK_LEFT:
                    case SDLK_RIGHT:
                    case SDLK_UP:
                    case SDLK_DOWN:
					case SDLK_a:
					case SDLK_b:
					case SDLK_x:
					case SDLK_y:
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
			if (PlayerStates[0])
			{
				LivesLost = 3;
				Score = 888888;
			}
			else
			{
				LivesLost = 0;
				Score = 0;
			}
		}
		DrawGame();
		DrawScoreBar(!PlayerStates[0]);
        SDL_FillRect(Screen,NULL,SDL_MapRGB(Screen->format,0,0,0));
        SDL_BlitSurface(Buffer,NULL,Screen,NULL);
		SDL_Flip(Screen);
		SDL_Delay(WaitForFrame());
	}

}

void LoadSettings()
{
 	FILE *SettingsFile;
 	SettingsFile = fopen("./settings.dat","r");
 	if(SettingsFile)
 	{		
		fclose(SettingsFile);
 	}
}

// Save the settings
void SaveSettings()
{
 	FILE *SettingsFile;
 	SettingsFile = fopen("./settings.dat","w");
 	if(SettingsFile)
 	{       
		fclose(SettingsFile);
		//sync();
 	}
}

int main(int argc, char **argv)
{
	bool useSoftwareRenderer = true;
    bool useFullScreenAtStartup = false;
    int c;
	while ((c = getopt(argc, argv, "?lafvo")) != -1) 
    {
        switch (c) 
        {
            case '?':
                // i use sdl log because on windows printf did not show up
                printf("Usage: formula [Options]\n\n\
Possible options are:\n\
  -?: show this help message\n\
  -a: Use Acclerated Renderer\n\
  -f: Run fullscreen at startup (by default starts up windowed)\n");
                exit(0);
                break;
            case 'a':
                useSoftwareRenderer = false;
                break;
            case 'f':
                useFullScreenAtStartup = true;
                break;
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
        if (useFullScreenAtStartup) {
            Flags |= SDL_FULLSCREEN;            
        }
		if (useSoftwareRenderer) 
			Flags |= SDL_SWSURFACE;
		else
			Flags |= SDL_HWSURFACE;
		Screen = SDL_SetVideoMode( WINDOW_WIDTH, WINDOW_HEIGHT,32, Flags );
		if(Screen)
		{
		    Buffer = SDL_CreateRGBSurface(SDL_HWSURFACE,WINDOW_WIDTH,WINDOW_HEIGHT,32,Screen->format->Rmask,Screen->format->Gmask,Screen->format->Bmask,Screen->format->Amask);
			printf("Succesfully Set %dx%dx32\n",WINDOW_WIDTH,WINDOW_HEIGHT);
			SDL_ShowCursor(SDL_DISABLE);
			if (Mix_OpenAudio(11025,AUDIO_S16,MIX_DEFAULT_CHANNELS,64) < 0)
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


