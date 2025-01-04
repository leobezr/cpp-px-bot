#pragma once
#include "./Movement.h";
#include "../Constants/Hotkeys.h";

class Player
{
public:
	void cast(WORD key)
	{
		this->__movement.press(key);
	}

private:
	Movement __movement;
};

