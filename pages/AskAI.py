import plotly.express as px
import plotly.graph_objects as go
import pandas as pd 
import mysql.connector
import random
import json
import requests
import streamlit as st
from streamlit_lottie import st_lottie
from pandasai import SmartDataframe
from pandasai.llm import OpenAI
import os

@st.cache_resource
def init_connection():
    return mysql.connector.connect(**st.secrets["mysql"])

# Connect to the database
conn = init_connection()
os.environ["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"] = "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"

@st.cache_data(ttl=600)
def run_query(query):
    with conn.cursor() as cur:
        cur.execute(query)
        return cur.fetchall()
    
st.title("AI Data Analytics")
    
# Query to fetch data for all people
query_all = "SELECT Liters, Time, Temperature, PersonType FROM data;"
data_all = run_query(query_all)
df_all = pd.DataFrame(data_all, columns=['Liters', 'Time', 'Temperature', 'PersonType'])

with st.expander("Dataframe Preview"):
    st.write(df_all)


ask = st.text_area("Chat with the AI")
st.write(ask)

if ask:
    llm = OpenAI(api_token=os.environ["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"])
    query_engine = SmartDataframe(df_all, config={"llm": llm})
    answer = query_engine.chat(ask)
    st.write(answer)