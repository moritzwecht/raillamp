// Auto-Refresh Status
setInterval(fetchStatus, 2000);
fetchStatus();

async function fetchStatus() {
    try {
        const res = await fetch('/status');
        const data = await res.json();
        updateUI(data);
    } catch (e) {
        showError("Verbindung verloren!");
    }
}

function updateUI(data) {
    // Lampen Status
    const icon = document.getElementById('lampIcon');
    const text = document.getElementById('statusText');
    if (data.lightsOn) {
        icon.className = 'lamp-icon on';
        text.className = 'status-text on';
        text.textContent = 'AN';
    } else {
        icon.className = 'lamp-icon';
        text.className = 'status-text off';
        text.textContent = 'AUS';
    }

    // Aktuelle Uhrzeit
    document.getElementById('currentTime').textContent = data.currentTime;

    // Parameter
    document.getElementById('brightnessValue').textContent =
        Math.round((data.brightness / 255) * 100) + '%';
    document.getElementById('maxBrightnessValue').textContent =
        data.maxBrightness;
    document.getElementById('timeoutValue').textContent =
        data.timeout + 's';
    document.getElementById('pir1Value').textContent =
        data.pir1 ? '✓ Aktiv' : '— Ruhig';
    document.getElementById('pir1Value').style.color =
        data.pir1 ? '#55ff55' : '#888';
    document.getElementById('pir2Value').textContent =
        data.pir2 ? '✓ Aktiv' : '— Ruhig';
    document.getElementById('pir2Value').style.color =
        data.pir2 ? '#55ff55' : '#888';

    // Status Meldung
    document.getElementById('statusMsg').textContent = data.status;
    showError(data.error);

    // Zeitplan Status
    const scheduleIndicator = document.getElementById('scheduleIndicator');
    const scheduleStatusText = document.getElementById('scheduleStatusText');

    if (data.scheduleEnabled) {
        if (data.withinSchedule) {
            scheduleIndicator.className = 'schedule-indicator active';
            scheduleStatusText.textContent = 'Aktiv (im Zeitfenster)';
        } else {
            scheduleIndicator.className = 'schedule-indicator inactive';
            scheduleStatusText.textContent = 'Wartet auf Zeitfenster';
        }
    } else {
        scheduleIndicator.className = 'schedule-indicator';
        scheduleStatusText.textContent = 'Zeitplan deaktiviert';
    }

    // Zeitplan Eingaben synchronisieren (nur wenn nicht gerade bedient)
    if (document.activeElement.id !== 'scheduleStart') {
        const startTime = pad(data.scheduleStartHour) + ':' + pad(data.scheduleStartMinute);
        document.getElementById('scheduleStart').value = startTime;
    }
    if (document.activeElement.id !== 'scheduleEnd') {
        const endTime = pad(data.scheduleEndHour) + ':' + pad(data.scheduleEndMinute);
        document.getElementById('scheduleEnd').value = endTime;
    }

    // Checkbox synchronisieren
    document.getElementById('scheduleEnabled').checked = data.scheduleEnabled;

    // Scharf-Schaltung Status
    const armedIndicator = document.getElementById('armedIndicator');
    const armedStatusText = document.getElementById('armedStatusText');
    const disarmBtn = document.getElementById('disarmBtn');

    if (data.isArmed) {
        armedIndicator.className = 'armed-indicator active';
        armedStatusText.textContent = 'Aktiv (noch ' + data.armedRemaining + ' Min.)';
        disarmBtn.className = 'btn btn-disarm visible';
    } else {
        armedIndicator.className = 'armed-indicator';
        armedStatusText.textContent = 'Nicht aktiv';
        disarmBtn.className = 'btn btn-disarm';
    }

    // Slider synchronisieren (nur wenn nicht gerade bedient)
    if (document.activeElement.id !== 'brightnessSlider') {
        document.getElementById('brightnessSlider').value = data.maxBrightness;
        document.getElementById('brightnessSliderVal').textContent = data.maxBrightness;
    }
    if (document.activeElement.id !== 'timeoutSlider') {
        document.getElementById('timeoutSlider').value = data.timeout;
        document.getElementById('timeoutSliderVal').textContent = data.timeout;
    }
}

function pad(num) {
    return num.toString().padStart(2, '0');
}

function showError(msg) {
    const el = document.getElementById('errorMsg');
    if (msg && msg !== '') {
        el.textContent = '⚠️ ' + msg;
        el.className = 'error-msg visible';
    } else {
        el.className = 'error-msg';
    }
}

let toastTimer = null;
function showToast(message, isError = false) {
    const toast = document.getElementById('toast');
    toast.textContent = message;
    toast.className = 'toast visible' + (isError ? ' error' : '');
    if (toastTimer) clearTimeout(toastTimer);
    toastTimer = setTimeout(() => {
        toast.className = 'toast';
    }, 2000);
}

async function fetchWithToast(url, okMessage, errorPrefix) {
    try {
        const res = await fetch(url);
        if (!res.ok) {
            const text = await res.text();
            showToast((errorPrefix ? errorPrefix + ': ' : '') + text, true);
            return false;
        }
        if (okMessage) showToast(okMessage);
        return true;
    } catch (e) {
        showToast((errorPrefix ? errorPrefix + ': ' : '') + 'Netzwerkfehler', true);
        return false;
    }
}

// Helligkeit Slider
document.getElementById('brightnessSlider').addEventListener('input', function() {
    document.getElementById('brightnessSliderVal').textContent = this.value;
    fetch('/set/brightness/' + this.value);
});

// Timeout Slider
document.getElementById('timeoutSlider').addEventListener('input', function() {
    document.getElementById('timeoutSliderVal').textContent = this.value;
    fetch('/set/timeout/' + this.value);
});

// Zeitplan speichern
document.getElementById('saveScheduleBtn').addEventListener('click', function() {
    const startTime = document.getElementById('scheduleStart').value;
    const endTime = document.getElementById('scheduleEnd').value;

    const [startH, startM] = startTime.split(':').map(Number);
    const [endH, endM] = endTime.split(':').map(Number);

    fetchWithToast(
        '/set/schedule/' + startH + '/' + startM + '/' + endH + '/' + endM,
        'Zeitplan gespeichert',
        'Zeitplan'
    );
});

// Zeitplan aktivieren/deaktivieren
document.getElementById('scheduleEnabled').addEventListener('change', function() {
    fetchWithToast(
        '/set/schedule/enabled/' + (this.checked ? '1' : '0'),
        this.checked ? 'Zeitplan aktiviert' : 'Zeitplan deaktiviert',
        'Zeitplan'
    );
});

// Scharf schalten Funktionen
async function armFor(hours) {
    await fetchWithToast('/arm/' + hours, 'Scharf für ' + hours + 'h', 'Scharf');
}

async function armForDay() {
    await fetchWithToast('/arm/day', 'Scharf bis Tagesende', 'Scharf');
}

async function disarm() {
    await fetchWithToast('/disarm', 'Scharf-Schaltung deaktiviert', 'Scharf');
}
