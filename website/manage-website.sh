#!/bin/bash

# Axiom Website Management Script

case "$1" in
    "start")
        echo "Starting Axiom website..."
        sudo docker stop nextcloud-talk-nginx-1 2>/dev/null || true
        sudo docker stop axiom-site 2>/dev/null || true
        sudo docker rm axiom-site 2>/dev/null || true
        sudo docker run -d -p 80:80 --name axiom-site axiom-website
        echo "✅ Axiom website started on port 80"
        echo "🌐 Local access: http://localhost"
        echo "🌐 External access: http://AxiomCompozitor.duckdns.org"
        ;;
    "stop")
        echo "Stopping Axiom website..."
        sudo docker stop axiom-site
        sudo docker rm axiom-site
        echo "✅ Axiom website stopped"
        ;;
    "restart")
        echo "Restarting Axiom website..."
        $0 stop
        $0 start
        ;;
    "status")
        echo "Checking website status..."
        if sudo docker ps | grep axiom-site >/dev/null; then
            echo "✅ Axiom website is running"
            sudo docker ps | grep axiom-site
        else
            echo "❌ Axiom website is not running"
        fi
        ;;
    "logs")
        echo "Showing website logs..."
        sudo docker logs axiom-site
        ;;
    "restore-nextcloud")
        echo "Restoring Nextcloud..."
        sudo docker stop axiom-site 2>/dev/null || true
        sudo docker rm axiom-site 2>/dev/null || true
        sudo docker start nextcloud-talk-nginx-1
        echo "✅ Nextcloud restored"
        ;;
    *)
        echo "Axiom Website Manager"
        echo "Usage: $0 {start|stop|restart|status|logs|restore-nextcloud}"
        echo ""
        echo "Commands:"
        echo "  start             - Start the Axiom website on port 80"
        echo "  stop              - Stop the Axiom website"
        echo "  restart           - Restart the Axiom website"
        echo "  status            - Check if the website is running"
        echo "  logs              - Show website logs"
        echo "  restore-nextcloud - Stop Axiom and restore Nextcloud"
        ;;
esac
