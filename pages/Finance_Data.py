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
    
st.title("Finance and Energy Data Analytics")
lottie_hello = load_lottieurl("https://lottie.host/95f4da18-1a11-4e32-9218-c3760a5f1ba1/EWkjGnYuKp.json")

st_lottie(lottie_hello, loop=True, width=700, height=300)

person_type_names = {1: "Novel", 2: "Brian", 3: "Shinika", 4: "Jordan"}

# Get unique person types from the database
person_types_query = "SELECT DISTINCT PersonType FROM data;"
person_types = [type[0] for type in run_query(person_types_query)]

cost_per_liter = st.number_input("Enter Cost Per Liter (in dollars):", min_value=0.0, value=0.0, step=0.01)

cost_per_liter_dict = {}

person_type_count = {}

# Loop through each person type
for person_type in person_types:
    query_selected = f"SELECT Liters, Time, Temperature, PersonType FROM data WHERE PersonType = '{person_type}';"
    data_selected = run_query(query_selected)
    df_selected = pd.DataFrame(data_selected, columns=['Liters', 'Time', 'Temperature', 'PersonType'])

    df_selected['Cost'] = cost_per_liter * df_selected['Liters']

    person_name = person_type_names.get(person_type, f"Person {person_type}")
    st.subheader(f"Cost per Liter for {person_name}:")
    st.write(df_selected[['PersonType', 'Cost']])

    cost_per_liter_dict[person_type] = df_selected['Cost'].iloc[0]

    person_type_count[person_type] = len(df_selected)

# Display total cost for all people
total_cost_dollars = sum(cost_per_liter_dict.values())
st.subheader("Total Cost for All People:")
st.write(f"${total_cost_dollars:.2f}")

st.subheader("Breakdown of Costs for Each Person:")
for person_type, cost in cost_per_liter_dict.items():
    person_name = person_type_names.get(person_type, f"Person {person_type}")
    st.write(f"{person_name}: ${cost:.2f} (Count: {person_type_count[person_type]})")

st.subheader("Total Showers:")
for person_type, total_showers in person_type_count.items():
    person_name = person_type_names.get(person_type, f"Person {person_type}")
    st.write(f"{person_name}: Showers Total: {total_showers}")

st.header("Energy Usage")

# Query data for energy usage
energy_query = "SELECT PersonType, SUM((4.2 * Liters * 20) / 3600) as TotalEnergy FROM data GROUP BY PersonType;"
energy_data = run_query(energy_query)
df_energy = pd.DataFrame(energy_data, columns=['PersonType', 'TotalEnergy'])

# Create a bar chart using Plotly Express with different colors for each person type
color_sequence = px.colors.qualitative.Set1  # You can choose a different color sequence
fig_energy = px.bar(df_energy, x='PersonType', y='TotalEnergy', color='PersonType', color_discrete_sequence=color_sequence, labels={'TotalEnergy': 'Total Energy Usage'})
fig_energy.update_layout(title='Total Energy Usage by Person Type', xaxis_title='Person Type', yaxis_title='Total Energy (KWH)')

# Display the bar chart in the Streamlit app
st.plotly_chart(fig_energy)
