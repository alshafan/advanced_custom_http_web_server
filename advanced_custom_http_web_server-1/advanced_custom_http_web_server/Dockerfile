FROM gcc:latest

WORKDIR /app

# Copy all source files and public directory into the container
COPY . .

# Compile the C server
RUN gcc -Wall -Wextra -O2 -o server server.c

# Render uses port 10000 by default
EXPOSE 10000

# Run the compiled binary
CMD ["./server"]
