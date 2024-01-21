import plotly.express as px
import plotly.graph_objects as go
import pandas as pd 
import mysql.connector
import random
import json
import requests
import streamlit as st
from streamlit_lottie import st_lottie

@st.cache_resource
def init_connection():
    return mysql.connector.connect(**st.secrets["mysql"])

conn = init_connection()

@st.cache_data(ttl=600)
def run_query(query):
    with conn.cursor() as cur:
        cur.execute(query)
        return cur.fetchall()
    
def load_lottieurl(url: str):
    r = requests.get(url)
    if r.status_code != 200:
        return None
    return r.json()

person_type_names = {1: "Novel", 2: "Brian", 3: "Shinika", 4: "Jordan"}

person_types_query = "SELECT DISTINCT PersonType FROM data;"
person_types = [type[0] for type in run_query(person_types_query)]

st.title("HydroSense Data Analytics")
lottie_hello = load_lottieurl("https://lottie.host/543507b2-f845-4573-b93f-cef085bdf2bf/paxE6qfziA.json")

st_lottie(lottie_hello, loop=True, width=700, height=300)

query_all = "SELECT Liters, Time, Temperature, PersonType FROM data;"
data_all = run_query(query_all)
df_all = pd.DataFrame(data_all, columns=['Liters', 'Time', 'Temperature', 'PersonType'])

fig_liters_all = px.scatter(df_all, x='Time', y='Liters', color='PersonType', trendline="ols", title='Liters vs Time for All Individuals in Household', labels={'Liters': 'Liters (L)', 'Time': 'Time'})

fig_liters_all.add_shape(
    go.layout.Shape(
        type="line",
        x0=df_all['Time'].min(),
        x1=df_all['Time'].max(),
        y0=75,
        y1=75,
        line=dict(color="red", width=2),
    )
)

fig_liters_all.update_traces(marker=dict(color=[f'#{random.randint(0, 0xFFFFFF):06x}' for _ in range(len(df_all))]), showlegend=False)
st.plotly_chart(fig_liters_all)

fig_temperature_all = px.scatter(df_all, x='Temperature', y='Liters', color='PersonType', trendline="ols", title='Temperature vs Liters for All Individuals in Household', labels={'Temperature': 'Temperature (°C)', 'Liters': 'Liters (L)'})

# Add a red line at 75 liters
fig_temperature_all.add_shape(
    go.layout.Shape(
        type="line",
        x0=df_all['Temperature'].min(),
        x1=df_all['Temperature'].max(),
        y0=75,
        y1=75,
        line=dict(color="red", width=2),
    )
)

fig_temperature_all.update_traces(marker=dict(color=[f'#{random.randint(0, 0xFFFFFF):06x}' for _ in range(len(df_all))]), showlegend=False)
st.plotly_chart(fig_temperature_all)

st.header("Individual Milestones")

st.subheader("Usage Analytics:")
shortest_bath_person = df_all.loc[df_all['Time'].idxmin()]
coldest_bath_person = df_all.loc[df_all['Temperature'].idxmin()]
least_liters_person = df_all.loc[df_all['Liters'].idxmin()]

shortest_bath_person_name = person_type_names.get(shortest_bath_person['PersonType'], f"Person {shortest_bath_person['PersonType']}")
coldest_bath_person_name = person_type_names.get(coldest_bath_person['PersonType'], f"Person {coldest_bath_person['PersonType']}")
least_liters_person_name = person_type_names.get(least_liters_person['PersonType'], f"Person {least_liters_person['PersonType']}")

st.write(f"Shortest shower individual:")
st.write(f"- Person: {shortest_bath_person_name}")
st.write(f"- Time: {shortest_bath_person['Time']} minutes")

st.write(f"Least heat shower individual:")
st.write(f"- Person: {coldest_bath_person_name}")
st.write(f"- Temperature: {coldest_bath_person['Temperature']} °C")

st.write(f"Water-saving shower individual:")
st.write(f"- Person: {least_liters_person_name}")
st.write(f"- Liters: {least_liters_person['Liters']} Liters")

# Average Values section
st.header("Average Values")

# Calculate and display average total time, liters, and temperature
st.subheader("Average Total Time: (in minutes)")
st.write(df_all['Time'].mean())

st.subheader("Average Total Temperature: (in celcius)")
st.write(df_all['Temperature'].mean())

st.subheader("Average Total Liters:")
st.write(df_all['Liters'].mean())

lottie_hello1 = load_lottieurl("https://lottie.host/6a36fd81-4f6a-4d2e-b31f-86ca3f542a83/gabbKRET4U.json")

st_lottie(lottie_hello1, loop=True, width=700, height=300)


# Notes: C:\Users\shini\HydroSense is directory for streamlit application - Need admin to run on Anaconda Portal - Need to run first two systems for MySQL