#! /usr/bin/env python

from flask import Flask, render_template, request
from flask_wtf import FlaskForm
from wtforms import StringField, SubmitField
from wtforms.fields import IntegerRangeField
import time
import os

app = Flask(__name__)
app.config['SECRET_KEY'] = 'filesystem'

class UpdateForm(FlaskForm):
    brightness = IntegerRangeField('Brightness')
    content = StringField('Text')
    submit = SubmitField('✅ Update')
    shutdown = SubmitField('💤 Night Night')

@app.route("/", methods=['GET', 'POST'])
def main():
    form = UpdateForm()

    if not os.path.exists("content"):
        os.mknod("content");
    if not os.path.exists("brightness"):
        os.mknod("brightness");

    if request.method == 'GET':
        with open("brightness", "r") as fd:
            brightness = fd.readlines()
            form.brightness.data = int(brightness[0]) if brightness else 0
        with open("content", "r") as fd:
            content = fd.readlines()
            form.content.data = content[0] if content else ""
        return render_template("index.html", form=form, brightness=form.brightness.data)

    if form.validate_on_submit():
        brightness = 0
        if form.brightness.data and not form.shutdown.data:
            brightness = form.brightness.data
        with open("brightness", "w") as fd:
            fd.write(str(brightness));

        content = ""
        if form.content.data:
            content = form.content.data
        with open("content", "w") as fd:
            fd.write(form.content.data);

        return render_template("index.html", form=form, brightness=brightness)

@app.route("/brightness", methods=['GET'])
def brightness():
    h = time.localtime().tm_hour
    age = int((time.time() - os.path.getmtime("brightness")) / 60)
    if (h >= 23 or h < 6) and age >= 60:
        brightness = 0
        with open("brightness", "w") as fd:
            fd.write(str(brightness));
        return str(brightness)
    else:
        with open("brightness", "r") as fd:
            brightness = int(fd.readlines()[0])
        return str(brightness)

@app.route("/content", methods=['GET'])
def content():
    with open("content", "r") as fd:
        content = fd.readlines()[0]
    return content

if __name__ == "__main__":
    app.run(host='0.0.0.0')
