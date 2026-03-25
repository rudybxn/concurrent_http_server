# AI Reflections - Team 0101

## Helena Zhuo
    During this sprint, I used Claude to help me with the skeleton code, information about best practices for file organization, and the README.md format and best practices there as well. For the skeleton code, I prompted Claude with the information I had gathered from reviewing source code from the Apache HTTP server and the NGINX server, and my notes I had taken from the OSTEP textbook and my own research. I specifically prompted Claude for a networking http.c file, a main.c file that handled the logic and was singly-threaded for the time being, and a stubbed thread pool file that we would implement in later sprints, with extensive comments for our understanding. I also generated a mock client file so we could test our server. I asked our team members to review the code, especially the comments so that we all understand it thoroughly. For the README.md format and file organization, I asked Claude for an outline which I then filled in and provided for my team to fill out further.
## Ishit Arhatia
    For this sprint, I took help of Claude to help me format the annotated bibliography in markdown format. I prompted claude with all our findings from different sources and just put it in an ordered manner.
## Tomas Colorado
    For this sprint, I didn't use AI to assist me in my tasks.
## Rudrajit Banerjee
    For sprint 3, I used AI to plan and organize code for the backend functionality of the server. I used it to generate boilerplate code upon which I made changes with respect to the functionality mentioned in our research proposal and concepts I learned in OSTEP. I also used it to reason about architectural decisions (like why the bounded buffer should be separate from the thread pool, and why the buffer should hold request_t structs instead of ints).  I also used it to understand unfamiliar POSIX APIs which I had not come across before (pthread_cond_wait, sigaction, stat).
