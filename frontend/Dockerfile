FROM tiangolo/uwsgi-nginx-flask:latest

COPY ./requirements.txt /app/requirements.txt

RUN pip install --no-cache-dir --upgrade -r /app/requirements.txt

COPY ./app.py /app/main.py
COPY ./templates /app/templates
COPY ./emojis /app/emojis
ENV STATIC_PATH /app/emojis
ENV STATIC_URL /emojis
