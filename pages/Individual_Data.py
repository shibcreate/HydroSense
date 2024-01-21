import plotly.express as px
import plotly.graph_objects as go
import pandas as pd 
import mysql.connector
import random
import json
import requests
import streamlit as st
from streamlit_lottie import st_lottie
st.markdown("# Individual Data Statistics")

# Lottie animation
st_lottie_url = "https://lottie.host/64f9e01d-7fcb-4e84-9c4b-7ccda90ef287/qDWRcquCX1.json"
st_lottie(st_lottie_url, width=700, height=300, speed=1, key="individual_stats_lottie")

import plotly.express as px
import pandas as pd 
import mysql.connector
import random

# Function to initialize connection to the database
@st.cache_resource
def init_connection():
    return mysql.connector.connect(**st.secrets["mysql"])

# Connect to the database
conn = init_connection()

# Function to run query and fetch data
@st.cache_data(ttl=600)
def run_query(query):
    with conn.cursor() as cur:
        cur.execute(query)
        return cur.fetchall()

# Mapping numeric values to names
person_type_mapping = {1: 'Novel', 2: 'Brian', 3: 'Shinika', 4: 'Jordan'}

person_types_query = "SELECT DISTINCT PersonType FROM data;"
person_types = [person_type_mapping[type[0]] for type in run_query(person_types_query)]

# Dropdown menu for selecting person type
selected_person = st.selectbox("Select Person Type:", person_types)

selected_person_numeric = next(key for key, value in person_type_mapping.items() if value == selected_person)

query = f"SELECT Liters, Time, Temperature, PersonType FROM data WHERE PersonType = {selected_person_numeric};"
data = run_query(query)
df = pd.DataFrame(data, columns=['Liters', 'Time', 'Temperature', 'PersonType'])

fig_liters = px.scatter(df, x='Time', y='Liters', title=f'Liters vs Time for {selected_person}', labels={'Liters': 'Liters (L)', 'Time': 'Time'})
st.plotly_chart(fig_liters)

fig_temperature = px.scatter(df, x='Temperature', y='Liters', title=f'Temperature vs Liters for {selected_person}', labels={'Temperature': 'Temperature (°C)', 'Liters': 'Liters (L)'})
st.plotly_chart(fig_temperature)

st.header("Average Values")

average_dropdown = st.selectbox("Select Average Value:", ["Average Time", "Average Temperature", "Average Liters"])

# Calculate and display average values based on the selected dropdown
if average_dropdown == "Average Time":
    st.subheader("Average Time:")
    st.write(df['Time'].mean())

elif average_dropdown == "Average Temperature":
    st.subheader("Average Temperature:")
    st.write(df['Temperature'].mean())

elif average_dropdown == "Average Liters":
    st.subheader("Average Liters:")
    st.write(df['Liters'].mean())
