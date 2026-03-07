# AquaFlux

AQUA-Flux floating/autonomous chamber system designed to measure greenhouse gas fluxes (primarily CO₂ and CH₄) at air-water interfaces in aquatic environments (e.g., lakes, wetlands, ponds). This is an adaptation of methods described by Bastviken et al. (2020) and others, utilizing a floating chamber that periodically opens to allow ambient air exchange and then closes to accumulate gases for measurement. The system is designed for remote deployment and data logging in the field.

This setup logs data to SD card, transmits via XBee for remote access, controls a chamber lid via linear actuator (to periodically open for ambient air exchange and close for flux accumulation), reads multiple sensors, and includes basic error recovery for the I²C bus.