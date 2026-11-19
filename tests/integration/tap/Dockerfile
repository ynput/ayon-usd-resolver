FROM python:3.12-slim
RUN pip install --no-cache-dir "aiohttp>=3.9" \
    opentelemetry-sdk opentelemetry-exporter-otlp-proto-http
COPY ayon_tap.py /app/ayon_tap.py
EXPOSE 5000
HEALTHCHECK --interval=5s --timeout=2s --retries=5 \
    CMD python -c "import urllib.request; urllib.request.urlopen('http://localhost:5000/__tap/counts')"
CMD ["python", "/app/ayon_tap.py"]
