#!/bin/bash

# Axiom Website Tunnel Manager

case "$1" in
    "start")
        echo "🚀 Starting Cloudflare tunnel for Axiom website..."
        echo "📋 Make sure your website is running first:"
        echo "   ./manage-website.sh start"
        echo ""
        echo "🌐 Starting tunnel to http://localhost:8081"
        cloudflared tunnel --url http://localhost:8081
        ;;
    "background")
        echo "🚀 Starting Cloudflare tunnel in background..."
        cloudflared tunnel --url http://localhost:8081 > tunnel.log 2>&1 &
        echo $! > tunnel.pid
        echo "✅ Tunnel started in background"
        echo "📄 Logs: cat tunnel.log"
        echo "🛑 Stop: ./tunnel.sh stop"
        sleep 3
        echo "🌐 Your tunnel URL:"
        grep "trycloudflare.com" tunnel.log | tail -1
        ;;
    "stop")
        if [ -f tunnel.pid ]; then
            PID=$(cat tunnel.pid)
            kill $PID 2>/dev/null
            rm tunnel.pid
            echo "✅ Tunnel stopped"
        else
            echo "❌ No tunnel running"
        fi
        ;;
    "status")
        if [ -f tunnel.pid ]; then
            PID=$(cat tunnel.pid)
            if kill -0 $PID 2>/dev/null; then
                echo "✅ Tunnel is running (PID: $PID)"
                echo "🌐 Your tunnel URL:"
                grep "trycloudflare.com" tunnel.log | tail -1 2>/dev/null || echo "Check tunnel.log for URL"
            else
                echo "❌ Tunnel process not found"
                rm tunnel.pid
            fi
        else
            echo "❌ No tunnel running"
        fi
        ;;
    "logs")
        if [ -f tunnel.log ]; then
            tail -f tunnel.log
        else
            echo "❌ No tunnel logs found"
        fi
        ;;
    *)
        echo "🌐 Axiom Website Tunnel Manager"
        echo "Usage: $0 {start|background|stop|status|logs}"
        echo ""
        echo "Commands:"
        echo "  start      - Start tunnel (foreground)"
        echo "  background - Start tunnel (background)"
        echo "  stop       - Stop background tunnel"
        echo "  status     - Check tunnel status"
        echo "  logs       - Show tunnel logs"
        ;;
esac
