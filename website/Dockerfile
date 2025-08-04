# Use the official Nginx image based on Alpine Linux
FROM nginx:alpine

# Copy the website files to the Nginx html directory
COPY index.html /usr/share/nginx/html/

# Copy a custom nginx configuration (optional)
COPY nginx.conf /etc/nginx/nginx.conf

# Expose port 80
EXPOSE 80

# Start Nginx server
CMD ["nginx", "-g", "daemon off;"]
