# Build notes - 2024-12-03

## Engine

No new modules were added to the engine in the last 2 months - infact not much work
went into this repo, I've been jugling every other class and as I was further ahead
in this class I put most of my work on hold here.

Im happy with the maturity of the engine, test coverage has increased at an acceptable
rate. I've been moving from catch2 onto googletest because I got sick of all the stupid
things catch2 tries to do - redirecting stdout/stderr, installing vectored exception handlers,
trying to catch SEH signals on windows, and posix signals on linux. googletest just gets
out the way.

I no longer require a custom fork of clang, which helped ease the port to linux.
On windows a fork of clang is *technically* required, but only because meson doesnt
know about `-xclang=-std=gnu++26`. A very simple opengl + glfw backend was added
as well to aid with porting to linux, i tried using glfw on windows but its interactions
with imgui were undesirable and it doesnt play nicely with my input handling model.

## The game

I was able to complete all the goals of my game aside from audio, after months of working
on the engine the actual game was done in under 3 days. I actually used all the engine
facilities I developed over the term - the db and dao were used for logging and scoring,
my structured logging worked in providing visibility to engine crashes, and my new rendering
module was much nicer to work with than the old module. This allowed me to spend more time
working on optimizing shaders and adding features rather than fighting spaghetti.

## What went well

### Testing

Adding tests really helped me make changes without so much fear of regressions, the fast
build test debug cycle helped reduce the fatigue i get from manual exploratory testing.
I hope in the future to integrate fuzztest into my testing for some exploratory tests and
automated bug discovery.

### DAO/SQL

From a single pretty crappy C++ file using libxml I managed to derive alot of good value,
having a good structured form to write persistent data structures in fixed all my serialization
woes from my previous attempts. SQLite made everything alot easier and it was nice to have a single
library where I didnt ever need to worry that the library was the one at fault rather than me.

### Logging

Structured logging was also pretty useful, my solution was beyond overkill for this scale of game.
Having the benchmarks to prove that I could throw log messages into hot loops and use them
to debug race conditions without causing heisenbugs or performance regressions was nice.

### Managing complexity

After 5 months of pretty sporadic and unfocused development the engine has grown quite a few limbs.
I think I did a good job of managing the ball of mud architecture, modules have all been very orthogonal
and have had very little interplay. No unreal engine stuff like `OnScreenDebugMessage` being tied to the players
`IGenericTeamAgentInterface` (what a fun one that was to debug!).

## What didnt go well

### Porting to linux

Turns out gcc doesnt support like half the features I regularly use, even gcc trunk dies with
`sorry, unimplemented:` errors. Getting linux support was useful for testing, and important
for another class of mine that requires building on macos but it was perhaps more effort than
I would've wanted. It exposed alot of holes in my design as I used win32 specific apis all
over the place. I get why unreal has a seperate launch module for each platform now.

### Audio

Audio is hard and I left it for last, unfortunately I cannot pull an entire audio engine
out of my ass in a single day. So audio had to be cut. Lesson learned for next time...

### Managing build times

Turns out making each log message instantiate about a dozen templates causes scaling issues,
who would've thought. I really need to use type erasure more aggresively in the future.
I should also probably setup CI with `-ftime-trace` and `ninjatracing` to automate build
time reports for better diagnosis.

### Testing

I really didnt get the amount of coverage I might have wanted, Injecting test code in all
the places I would need it without leaking tons of implementation details is pretty tricky.
I think in the future I'll start adding private headers for testing per module.

### Time management

This seems to be my constant battle, it's been very hard to find time for every class
this semester without either missing assignments or handing in garbage work. In the future
I should probably plan work ahead of time more, and maybe be a little less optimisic
about time costs of features/projects.

### Documentation

Its a Sisyphean task, whenever I write docs theres never enough, and then a week later
they're all wrong and the time was wasted. So I put it off forever. I probably need to
keep track of last changed times on files and find a good point in time where the file
has matured enough to warrant stable documentation. I could probably tie this in with
testing as well, writing tests and documentation for a stable component is probably a
good use of time to iron out remaining bugs.
