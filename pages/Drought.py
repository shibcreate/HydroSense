import json
import streamlit as st
import mysql.connector
from urllib import parse, request

st.markdown("<h1 style='text-align: center; color: white;'>Drought Estimation and Usage</h1>", unsafe_allow_html=True)

# Function to initialize database connection
def init_connection():
    secrets = st.secrets["mysql"]
    return mysql.connector.connect(
        host=secrets["host"],
        port=secrets["port"],
        user=secrets["user"],
        password=secrets["password"],
        database=secrets["database"]
    )

# Function to make API request for weather forecast
def fetch_weather_forecast(city_state):
    url = f'https://api.aerisapi.com/forecasts/{parse.quote(city_state)}?format=json&filter=day&limit=7&fields=periods.dateTimeISO,loc,periods.maxTempF,periods.pop,periods.precipIN,periods.maxHumidity,periods.maxDewpointF,periods.weather&client_id=uTlj2O8JRDeX8LTrI6DC4&client_secret=0YxyOTlJCS96SuJdV03yWOjQS1safDOVWoWngW2V'
    
    try:
        req = request.urlopen(url)
        response = req.read()
        weather_data = json.loads(response)

        if weather_data['success']:
            return weather_data
        else:
            st.error(f"An error occurred: {weather_data['error']['description']}")
    except Exception as e:
        st.error(f"An error occurred: {str(e)}")
    finally:
        req.close()

# Function to estimate drought conditions
def estimate_drought(day_data):
    precipitation_threshold = 0.1 
    temperature_threshold = 90 
    humidity_threshold = 30  

    # Extract relevant information
    precipitation = day_data.get('precipIN', 0)
    temperature = day_data.get('maxTempF', 0)
    humidity = day_data.get('maxHumidity', 0)

    # Basic drought estimation criteria
    is_drought_day = precipitation < precipitation_threshold or temperature > temperature_threshold or humidity < humidity_threshold

    return is_drought_day

# Function to fetch most recent shower data from MySQL database based on PersonType
def fetch_mysql_data(person_type):
    conn = init_connection()

    query = f"SELECT Time, Liters, Temperature FROM data WHERE PersonType = {person_type} ORDER BY Time ASC LIMIT 1;"
    cursor = conn.cursor(dictionary=True)

    try:
        cursor.execute(query)
        data = cursor.fetchone()
        return data
    except Exception as e:
        st.error(f"An error occurred while fetching data from MySQL: {str(e)}")
    finally:
        cursor.close()
        conn.close()

def main():
    city_state = st.text_input("Enter City and State (e.g., san jose,ca):", "san jose,ca")
    
    person_type_names = {1: "Novel", 2: "Brian", 3: "Shinika", 4: "Jordan"}
    
    person_types = list(person_type_names.values())
    selected_person_type_name = st.selectbox("Select Person Type:", person_types)

    # Reverse mapping to get the corresponding number for the selected name
    selected_person_type = {v: k for k, v in person_type_names.items()}[selected_person_type_name]

    # Fetch weather forecast from API
    weather_data = fetch_weather_forecast(city_state)

    if weather_data and weather_data['success']:
        try:
            # Extract relevant information
            location = weather_data['response'][0]['loc']['name']
        except KeyError:
            location = "N/A"
        
        forecasts = weather_data['response'][0]['periods']

        # Display information for the first set only
        st.header(f"Weather Forecast for {location}")
        first_period = forecasts[0] if forecasts else {}
        st.write(f"Date: {first_period.get('dateTimeISO', 'N/A')}")
        st.write(f"Max Temperature: {first_period.get('maxTempF', 'N/A')} °F")
        st.write(f"Probability of Precipitation: {first_period.get('pop', 'N/A')}%")
        st.write(f"Precipitation: {first_period.get('precipIN', 'N/A')} inches")
        st.write(f"Max Humidity: {first_period.get('maxHumidity', 'N/A')}%")
        st.write(f"Max Dewpoint: {first_period.get('maxDewpointF', 'N/A')} °F")
        st.write(f"Weather: {first_period.get('weather', 'N/A')}")

        # Estimate drought conditions
        is_drought_day = estimate_drought(first_period)

        if is_drought_day:
            st.warning("Drought conditions are estimated for this day.")
            water_recommendation = "Use water conservatively. Consider reducing water usage today."
        else:
            st.info("No significant drought conditions are estimated this day.")
            water_recommendation = "Use water according to normal consumption guidelines."

        st.write("Water Consumption Recommendation:")
        st.write(water_recommendation)

        st.write("-" * 30)

        # Fetch most recent shower data from MySQL database based on selected PersonType
        mysql_data = fetch_mysql_data(selected_person_type)

        if mysql_data:
            st.header("Most Recent Shower:")
            st.write(f"Time: {mysql_data['Time']}")
            st.write(f"Liters: {mysql_data['Liters']} Liters")
            st.write(f"Person Type: {selected_person_type_name}")
            st.write(f"Temperature: {mysql_data['Temperature']} °C")

            # Check if Liters crossed the 10L threshold when drought is present
            if is_drought_day and mysql_data['Liters'] > 10:
                st.error("Your recent shower crossed the 10L threshold during drought conditions.")
        else:
            st.warning("No data available from the MySQL database.")
    else:
        st.warning("No weather data available.")

if __name__ == "__main__":
    main()
