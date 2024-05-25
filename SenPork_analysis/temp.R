if (!require(influxdbclient)) install.packages("influxdbclient")
if (!require(fpp3)) install.packages("fpp3")
if (!require(plotly)) install.packages("plotly")
library(influxdbclient)
library(plotly)
library(fpp3)
options(browser = "/usr/bin/chromium")

## You can generate an API token from the "API Tokens Tab" in the UI
## In your R console you can set token as
## Sys.setenv(INFLUX_TOKEN="onQUT_2bstVoLgE0HQ_pNzqiWCy1glOCFr0EiSypZdtKY4SX8yyGU13DoH-2BhXl2ydKffC0bGoSO3yBTFkMiQ==")
token <- Sys.getenv("INFLUX_TOKEN")

client <- InfluxDBClient$new(
  url = "http://iot.unloquer.org:8086",
  token = token,
  org = "unloquer"
)

fields <- c("CO2" = "co2", "Humedad" = "hum", "Material particulado" = "pm25", "Presión sonora" = "snd", "Temperatura" = "temp")

flux_filter_format <- function(named_measurement_tag_field = c("_field"="temp")) {
  glue::glue('r["{names(named_measurement_tag_field)}"] == "{named_measurement_tag_field}"')
}

flux_multiple_filters <- function(filter = c("_field" = "temp")) {
  purrr::map(filter, flux_filter_format) |> stringr::str_flatten(collapse = " or ")
}

flux_multiple_filters()
a <- flux_multiple_filters(c("sensor1", "sensor2") %>% purrr::set_names(c("_field","_field")))

a <- flux_filter_format(c("sensor1", "sensor2") %>% purrr::set_names(c("_field","_field")))

typeof(a)
## TODO:
##   - Desde el query que solo me retorne las columnas que necesito

query_builder_fn <- function(since_days = 1, fields_name = "temp", location_name = "experimento") {
  glue::glue('from(bucket: "senpork") |> range(start: -{since_days}d) |> filter(fn: (r) => r["_measurement"] == "tele") |> filter(fn: (r) => r["location"] == "{location_name}") |> filter(fn: (r) => {flux_multiple_fields(fields_name)}) |> aggregateWindow(every: 1h, fn: mean, createEmpty: false) |> yield(name: "mean")')
}

## LIBRO
## https://otexts.com/fpp3/tsibbles.html
##
tables <- client$query(query_builder_fn(80, fields)) |>
  purrr::map(tibble::tibble)

|>
  purrr::set_names(fields)

str(tables)

## TODO: deal with nanotime type
temperatures <- tsibble(
  measurement = tables$temp$`_value`,
  time = ymd(substr(tables$temp$`time`,1,19))
)

remove_duplicates <- duplicated(c("time"=substr(tables$temp$`time`,1,19)))
no_duplicates <- substr(tables$temp$`time`,1,19)
length(no_duplicates)


## https://github.com/tidyverse/ggplot2/issues/4288
g <- temperatures %>% autoplot()
ggplotly(g)

gg_season(temperatures |> fill_gaps())

ts1 <- ts1[-which(ts1$`_value` == ts1$`_value` |> max()), ]


capabilities()
dev.cur()

plot(1:100, runif(100))
