FROM python:3.11-slim

# Install system dependencies for C++ compilation
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

# Install pybind11 via pip
RUN pip install pybind11

# Create a non-root user for running the application
RUN useradd --create-home --shell /bin/bash feen

# Set work directory
WORKDIR /app

# Copy the entire repository
COPY . .

# Build C++ bindings
RUN mkdir build && cd build && \
    cmake .. -DFEEN_BUILD_PYTHON=ON && \
    make -j$(nproc)

# Run core physics tests to ensure environment integrity
RUN cd build && ctest --output-on-failure

# Install Python runtime dependencies
RUN pip install -r web/requirements.txt

# Transfer ownership of /app to the non-root user
RUN chown -R feen:feen /app

# Set PYTHONPATH to include the built bindings and the python source directory
ENV PYTHONPATH="/app/build/python:/app/python:${PYTHONPATH}"

# Expose port
EXPOSE 5000

# Switch to non-root user before starting the server
USER feen

# Start the web server
CMD ["gunicorn", "-w", "4", "-b", "0.0.0.0:5000", "web.app:app"]
