#pragma once

class ResurrectionAPI
{
public:
	virtual bool should_resurrect(RE::Actor*) const { return false; };
	virtual void resurrect(RE::Actor*){};
};

typedef void (*AddSubscriber_t)(std::unique_ptr<ResurrectionAPI> api);
