trap 'kill $(jobs -p)' EXIT; until nice ./rtes_final_assignment <your api key here> BINANCE:BTCUSDT BINANCE:ETHUSDT AAPL AMZN GOOG & wait; do
    echo "Program exited with exit code $?. Respawning.." >&2
        sleep 5
done