#include <SDLframework.h>
#include <iostream>

//enviroment variables
SDLFramework *_framework;									//internal, for error handling
bool SDLFramework::frCreated = false;

void SDLFramework::RunLoop(){
	Uint32 now=SDL_GetTicks(), interval=100;
	signed int timeLeft = 0;
	bool terminate=false, fatalError;

//event and pseudo-timer loop *2)
while (1){
	//Process errors and flush error stack - it's done first, for filtering constructor errors
	fatalError = false;
	while(!errors.empty()){
		if(errors.top().type == 2) fatalError=true;
		std::cout<< errors.top().str << '\n';
		errors.pop();
	}
	if(fatalError) return;	//exit on fatal error

	//Call time function
	timeLeft = interval-(SDL_GetTicks()-now);				//this 'now' is in fact 'last'
	if(timeLeft > 0) SDL_Delay(timeLeft);					//to ensure TimeEvent won't be late too much
	timeLeft = interval-(SDL_GetTicks()-now);				//this includes m-task overhead
	
	now=SDL_GetTicks();
	interval = interface->TimeEvent(timeLeft);

	//Process pending events
	SDL_Event evt;
	while(SDL_PollEvent(&evt)){
		switch (evt.type){
			case SDL_KEYUP:
			case SDL_KEYDOWN:
				interface->KeyboardEvent(&evt); break;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEMOTION:
				interface->MouseEvent(&evt); break;
			case SDL_VIDEOEXPOSE:
			case SDL_VIDEORESIZE:
			case SDL_ACTIVEEVENT:
				interface->WindowEvent(&evt); break;
			case SDL_QUIT:
				if(!interface->QuitEvent()) terminate = true;
				break;
			default:;
				interface->OtherEvent(&evt);
		}
	}
	if(terminate) return;

	//timeLeft = interval-(SDL_GetTicks()-now);
	//here was delay
}
}

bool SetError(const char *str, int type){				// *3b)
	if(!_framework) return false;

	_FrError dummy;
	_framework->errors.push(dummy);
	switch(type){
		case 0: _framework->errors.top().str = "Warning: "; break;
		case 1: _framework->errors.top().str = "Error: "; break;
		case 2: _framework->errors.top().str = "Fatal error: "; break;
	}
	_framework->errors.top().str += str;
	_framework->errors.top().type = type;
	return true;
}

int GetLastError(char *out, int size){
	if(!_framework) return -1;

	for(int i=0; i<size-1; i++){
		out[i]= _framework->errors.top().str.c_str()[i];
		if(!_framework->errors.top().str[i]) break;		//end of str
	}
	out[size-1]=NULL;
	return _framework->errors.top().type;
}

int SDLFramework::Init(						
		SDLFrInterface *inter,				//pointer to user-defined interface
		int _width,							//in pixels
		int _height,						//in pixels
		int bitsPP,							//bits per pixel, see *0a)
		Uint32 SdlFlags,					//flags for SDL_SetVideoMode, see *0a) (FULLSCREEN is set here)
		Uint32 initSystems,					//which SDL subsystems should be initialized
		GLParam *glPar,						//pointer to structure containing additional OpenGl parameters
		const char* title,					//title of the window
		const char* minTitle				//title of the window if minimized
						   )				//all passed arguments should be valid and verified before Init is called
{
	if(frCreated) return -1;							//Do not create another instance - it is singleton
	
	_framework = this;									//for error calls, stays set even if Init fails
	
	std::string err;
	if(SDL_Init(initSystems) == -1){					//initialize SDL
		err = "Init: initialization error: ";
		err += SDL_GetError();
		SetError(err.c_str(),2);						//SDL init failed
		return -1;
	}
														//create OpenGL window
	window = SDL_SetVideoMode(_width,_height,bitsPP,SdlFlags | SDL_OPENGL);
	if(!window){
		err = "Init: window creation error: ";
		err += SDL_GetError();
		SetError(err.c_str(),2);
		return -1;
	}
	width = _width; height = _height;

	SDL_WM_SetCaption(title,minTitle);					//set window caption
														//set GL Attributes with error check
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, glPar->redSize);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, glPar->greenSize);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, glPar->blueSize);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, glPar->alphaSize);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, glPar->bufferSize);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, glPar->doubleBuffer);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, glPar->depthSize);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, glPar->accRedSize);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, glPar->accGreenSize);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, glPar->accBlueSize);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, glPar->accAlphaSize);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, glPar->hwAcceleration);
	
	//error check
{
	GLParam p;
	char num[100];
	SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &p.redSize);
	SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &p.greenSize);
	SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &p.blueSize);
	SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &p.alphaSize);
	SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &p.bufferSize);
	SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &p.doubleBuffer);
	SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &p.depthSize);
	SDL_GL_GetAttribute(SDL_GL_ACCUM_RED_SIZE, &p.accRedSize);
	SDL_GL_GetAttribute(SDL_GL_ACCUM_GREEN_SIZE, &p.accGreenSize);
	SDL_GL_GetAttribute(SDL_GL_ACCUM_BLUE_SIZE, &p.accBlueSize);
	SDL_GL_GetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, &p.accAlphaSize);
	SDL_GL_GetAttribute(SDL_GL_ACCELERATED_VISUAL, &p.hwAcceleration);

	if(glPar->redSize != p.redSize){
		err = "Init: Setting GL attribute failed, SDL_GL_RED_SIZE ";
		_itoa_s(p.redSize,num,100,10);
		err+= num; err+=" got, "; 
		_itoa_s(glPar->redSize,num,100,10);
		err+= num; err+=" requested.";
		SetError(err.c_str(),1);
	}
	if(glPar->greenSize != p.greenSize){
		err = "Init: Setting GL attribute failed, SDL_GL_GREEN_SIZE ";
		_itoa_s(p.greenSize,num,100,10);
		err+= num; err+=" got, "; 
		_itoa_s(glPar->greenSize,num,100,10);
		err+= num; err+=" requested.";		
		SetError(err.c_str(),1);
	}
	if(glPar->blueSize != p.blueSize){
		err = "Init: Setting GL attribute failed, SDL_GL_BLUE_SIZE ";
		_itoa_s(p.blueSize,num,100,10);
		err+= num; err+=" got, "; 
		_itoa_s(glPar->blueSize,num,100,10);
		err+= num; err+=" requested.";
		SetError(err.c_str(),1);
	}
	if(glPar->alphaSize != p.alphaSize){
		err = "Init: Setting GL attribute failed, SDL_GL_ALPHA_SIZE ";
		_itoa_s(p.alphaSize,num,100,10);
		err+= num; err+=" got, "; 
		_itoa_s(glPar->alphaSize,num,100,10);
		err+= num; err+=" requested.";
		SetError(err.c_str(),1);
	}
	if(glPar->bufferSize != p.bufferSize){
		err = "Init: Setting GL attribute failed, SDL_GL_BUFFER_SIZE ";
		_itoa_s(p.bufferSize,num,100,10);
		err+= num; err+=" got, "; 
		_itoa_s(glPar->bufferSize,num,100,10);
		err+= num; err+=" requested.";
		SetError(err.c_str(),1);
	}
	if(glPar->doubleBuffer != p.doubleBuffer){
		err = "Init: Setting GL attribute failed, SDL_GL_DOUBLEBUFFER ";
		_itoa_s(p.doubleBuffer,num,100,10);
		err+= num; err+=" got, "; 
		_itoa_s(glPar->doubleBuffer,num,100,10);
		err+= num; err+=" requested.";
		SetError(err.c_str(),1);
	}
	if(glPar->depthSize != p.depthSize){
		err = "Init: Setting GL attribute failed, SDL_GL_DEPTH_SIZE ";
		_itoa_s(p.depthSize,num,100,10);
		err+= num; err+=" got, "; 
		_itoa_s(glPar->depthSize,num,100,10);
		err+= num; err+=" requested.";
		SetError(err.c_str(),1);
	}
	if(glPar->accRedSize != p.accRedSize){
		err = "Init: Setting GL attribute failed, SDL_GL_ACCUM_RED_SIZE ";
		_itoa_s(p.accRedSize,num,100,10);
		err+= num; err+=" got, "; 
		_itoa_s(glPar->accRedSize,num,100,10);
		err+= num; err+=" requested.";
		SetError(err.c_str(),1);
	}
	if(glPar->accGreenSize != p.accGreenSize){
		err = "Init: Setting GL attribute failed, SDL_GL_ACCUM_GREEN_SIZE ";
		_itoa_s(p.accGreenSize,num,100,10);
		err+= num; err+=" got, "; 
		_itoa_s(glPar->accGreenSize,num,100,10);
		err+= num; err+=" requested.";
		SetError(err.c_str(),1);
	}
	if(glPar->accBlueSize != p.accBlueSize){
		err = "Init: Setting GL attribute failed, SDL_GL_ACCUM_BLUE_SIZE ";
		_itoa_s(p.accBlueSize,num,100,10);
		err+= num; err+=" got, "; 
		_itoa_s(glPar->accBlueSize,num,100,10);
		err+= num; err+=" requested.";
		SetError(err.c_str(),1);
	}
	if(glPar->accAlphaSize != p.accAlphaSize){
		err = "Init: Setting GL attribute failed, SDL_GL_ACCUM_ALPHA_SIZE ";
		_itoa_s(p.accAlphaSize,num,100,10);
		err+= num; err+=" got, "; 
		_itoa_s(glPar->accAlphaSize,num,100,10);
		err+= num; err+=" requested.";
		SetError(err.c_str(),1);
	}
	if(glPar->hwAcceleration != p.hwAcceleration){
		err = "Init: Setting GL attribute failed, SDL_GL_ACCELERAETD_VISUAL ";
		_itoa_s(p.hwAcceleration,num,100,10);
		err+= num; err+=" got, "; 
		_itoa_s(glPar->hwAcceleration,num,100,10);
		err+= num; err+=" requested.";
		SetError(err.c_str(),1);
	}
}
	
	interface = inter;									//set interface pointer
	frCreated = true;									//initialization successful - First instance created
	return 1;
}