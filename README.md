# Project 1 - Simple Mail Client

- Name: Zachary Peterson
- Email: zacharypeterson775@u.boisestate.edu
- Class: CS425-001

## Known Bugs or Issues

Tests are not implimented at this time so there are no known issues.

## Experience

TODO: Describe your experience with the project (struggles, breakthroughs, etc.).
I had ran into a lot of issues while working on this project. To start, I started this project
way later than I should. This forced me to work on the project over three days where I put in
over 24 hours of coding. My first 6 hours were my trying to perfect the command line input.
This gave me any trouble as I had to account for many possible senarios. After I was satisfied
with the command line, I started working on implimenting the socket connections. When I started,
I did not understand how the getaddrinfo command linked with the rest of the socket framework.
After researching for 5 hours, I finaly figured out to convert the struct returned into a proper
socket that I could later connect to. After getting the sockets working, I then spend several,
hours trying to get the SMTP server to response to my requests. This took at least 6 more hours
of my time. After a short break, I decided to tear down my project and convert it to Object C
style. After spending 4+ hours on the conversion. I was finaly in a state to debug the remaining
issue. I this took all of the time I had to work on this project. If I were to try again, I would
start much earlier and try the Object C style approach. 

## Analysis

For this project, I was tasked with creating the SMTP client by spliting the project into three
layers. These layers consist of the Pure Helper Function, Session and Transport Functions, and 
the Socket Functions. The goal of this slit was to make testing easier by providing a functional
separation between componet. By separating the Session and Transport functions from the the 
Socket Functions, the sockets could be replaced with files. This would allow for accurate and 
stable testing. Addtionaly, by spliting the Helper Functions from the rest of the code, we are
able to test these Helper functions with improve the stabiltiy and correctness of the code itself.
