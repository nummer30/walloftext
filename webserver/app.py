#! /usr/bin/env python

from flask import Flask, render_template
from flask_wtf import FlaskForm
from wtforms import StringField, IntegerField, SubmitField

app = Flask(__name__)
app.config['SECRET_KEY'] = 'filesystem'

class UpdateForm(FlaskForm):
    brightness = IntegerField('Brightness')
    content = StringField('Text')
    submit = SubmitField('Update')

@app.route("/", methods=['GET', 'POST'])
def main():
    form = UpdateForm()

    if form.brightness.data:
        with open("brightness", "w") as fd:
            fd.write(str(form.brightness.data));

    if form.content.data:
        with open("content", "w") as fd:
            fd.write(form.content.data);
    return render_template("index.html", form=form)

if __name__ == "__main__":
    app.run(host='0.0.0.0')
