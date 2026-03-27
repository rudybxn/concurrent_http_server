# AI Reflections - Team 0101

## Helena Zhuo
    ### Sprint 2
    During this sprint, I used Claude to help me with the skeleton code, information about best practices for file organization, and the README.md format and best practices there as well. For the skeleton code, I prompted Claude with the information I had gathered from reviewing source code from the Apache HTTP server and the NGINX server, and my notes I had taken from the OSTEP textbook and my own research. I specifically prompted Claude for a networking http.c file, a main.c file that handled the logic and was singly-threaded for the time being, and a stubbed thread pool file that we would implement in later sprints, with extensive comments for our understanding. I also generated a mock client file so we could test our server. I asked our team members to review the code, especially the comments so that we all understand it thoroughly. For the README.md format and file organization, I asked Claude for an outline which I then filled in and provided for my team to fill out further.
    ### Sprint 3
    For Sprint 3, I primarily worked on the benchmarking deliverable, so my use of AI centered around the implementation of the benchmarking rig. I prompted Claude AI to help me understand the integration of the `wrk` load generator into our benchmarking rig, and it also suggested the `dd` tool to create .bin files which helped create a reproducible measurement environment. When I ran into issues where the server would refuse the connection as our benchmarking protocol called for multiple spin-ups and tear-downs of our server, Claude AI recommended the `netcat` tool to poll the server to confirm it was ready for use. I prompted Claude AI for changes to our benchmark .sh files in addition to manually reviewing and editing the files.
## Ishit Arhatia
    For this sprint, I took help of Claude to help me format the annotated bibliography in markdown format. I prompted claude with all our findings from different sources and just put it in an ordered manner.
## Tomas Colorado
    For this sprint, I didn't use AI to assist me in my tasks.
## Rudrajit Banerjee
    For this sprint, I didn't use AI for my tasks.
