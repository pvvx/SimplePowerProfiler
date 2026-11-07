import struct
import serial
import time
import numpy as np
import os
import json
import collections
from PySide6 import QtCore, QtGui, QtWidgets
from joulescope_ui.plugins import *

try:
    import pyqtgraph as pg
    PYQTGRAPH_AVAILABLE = True
    pg.setConfigOptions(antialias=False)
except ImportError:
    PYQTGRAPH_AVAILABLE = False
    print("⚠ pyqtgraph не установлен.")

try:
    from pyjls import Writer, SignalType, DataType
    PYJLS_AVAILABLE = True
except ImportError:
    PYJLS_AVAILABLE = False
    print("⚠ pyjls не установлен.")

CONFIG_DIR = os.path.expanduser("~/.joulescope")
CONFIG_FILE = os.path.join(CONFIG_DIR, "pp_ina228_usb.json")

DEFAULT_CONFIG = {
    "com_port": "COM13",
    "v_mult": 1.953125e-4,
    "v_offset": 0.0,
    "i_mult": 3.05175e-8,
    "i_offset": 0.0,
    "sample_rate": 11904.7619,
    "channel_mode": "Current & Voltage",
    "show_graphs": True,             # Показывать ли графики
    "auto_open_waveform": False      # Автооткрытие в Waveform
}

def load_config():
    os.makedirs(CONFIG_DIR, exist_ok=True)
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
                config = json.load(f)
                for key, value in DEFAULT_CONFIG.items():
                    if key not in config:
                        config[key] = value
                return config
        except Exception:
            pass
    return DEFAULT_CONFIG.copy()

def save_config(config):
    try:
        with open(CONFIG_FILE, 'w', encoding='utf-8') as f:
            json.dump(config, f, indent=4)
    except Exception as e:
        print(f"Ошибка сохранения конфигурации: {e}")
        
def format_with_prefix(value, unit_base):
    if value == 0:
        return f"0.000 {unit_base}"
    abs_val = abs(value)
    if unit_base == 'A':
        prefixes = [(1e3, 1e-3, 'kA'), (1, 1, 'A'), (1e-3, 1e3, 'mA'), (1e-6, 1e6, 'µA'), (1e-9, 1e9, 'nA'), (0, 1e12, 'pA')]
    else:
        prefixes = [(1e3, 1e-3, 'kV'), (1, 1, 'V'), (1e-3, 1e3, 'mV'), (1e-6, 1e6, 'µV'), (1e-9, 1e9, 'nV'), (0, 1e12, 'pV')]
    for threshold, multiplier, suffix in prefixes:
        if abs_val >= threshold:
            return f"{value * multiplier:.3f} {suffix}"
    return f"{value:.3f} {unit_base}"        


class PrefixAxisItem(pg.AxisItem if PYQTGRAPH_AVAILABLE else object):
    def __init__(self, unit_base, *args, **kwargs):
        if PYQTGRAPH_AVAILABLE:
            super().__init__(*args, **kwargs)
        self.unit_base = unit_base
    
    def tickStrings(self, values, scale, spacing):
        if not PYQTGRAPH_AVAILABLE:
            return [str(v) for v in values]
        if self.unit_base == 'A':
            prefixes = [(1e3, 1e-3, 'kA'), (1, 1, 'A'), (1e-3, 1e3, 'mA'), (1e-6, 1e6, 'µA'), (1e-9, 1e9, 'nA'), (0, 1e12, 'pA')]
        else:
            prefixes = [(1e3, 1e-3, 'kV'), (1, 1, 'V'), (1e-3, 1e3, 'mV'), (1e-6, 1e6, 'µV'), (1e-9, 1e9, 'nV'), (0, 1e12, 'pV')]
        
        strings = []
        for v in values:
            if v == 0:
                strings.append(f"0 {self.unit_base}")
                continue
            abs_v = abs(v)
            formatted = f"{v:.3g} {self.unit_base}"
            for threshold, multiplier, suffix in prefixes:
                if abs_v >= threshold:
                    formatted = f"{v * multiplier:.3g} {suffix}"
                    break
            strings.append(formatted)
        return strings


class SerialWorker(QtCore.QObject):
    batch_received = QtCore.Signal(object, object, object, str)
    status_changed = QtCore.Signal(str)
    
    def __init__(self):
        super().__init__()
        self.ser = None
        self._is_running = False
        self.port = DEFAULT_CONFIG["com_port"]
        self.baudrate = 115200
        self.v_mult = DEFAULT_CONFIG["v_mult"]
        self.v_offset = DEFAULT_CONFIG["v_offset"]
        self.i_mult = DEFAULT_CONFIG["i_mult"]
        self.i_offset = DEFAULT_CONFIG["i_offset"]
        self.sample_rate = DEFAULT_CONFIG["sample_rate"]
        self.channel_mode = DEFAULT_CONFIG["channel_mode"]
        self.start_time = None
        self.batch_size = 200

    @QtCore.Slot()
    def start_device(self):
        self._is_running = True
        self.status_changed.emit(f"Подключение к {self.port}...")
        self.start_time = time.time()
        try:
            self.ser = serial.Serial(self.port, baudrate=self.baudrate, timeout=1.0)
            QtCore.QThread.msleep(200)
            chnl_map = {"Current": 1, "Voltage": 2, "Current & Voltage": 3}
            chnl = chnl_map.get(self.channel_mode, 3)
            cmd = struct.pack('<BBBBHB', 5, 0x38, 1, chnl, int(round(self.sample_rate)), 0)
            self.ser.write(cmd)
            self.status_changed.emit(f"Запущено: {self.sample_rate} sps, {self.channel_mode}")
            self.start_time = time.time()
            #self.tps = 1.0 / float(self.sr_combo.currentText())
            self._read_loop()
        except serial.SerialException as e:
            self.status_changed.emit(f"Ошибка порта: {e}")
            self._is_running = False

    @QtCore.Slot()
    def stop_device(self):
        self._is_running = False
        if self.ser and self.ser.is_open:
            try:
                cmd = struct.pack('<BBB', 2, 0x38, 0)
                self.ser.write(cmd)
                self.ser.close()
                self.status_changed.emit("Устройство остановлено.")
            except Exception:
                pass

    def _read_loop(self):
        sample_index = 0
        ui_update_counter = 0
        v_batch, i_batch, t_batch = [], [], []
        
        while self._is_running and self.ser and self.ser.is_open:
            if self.ser.in_waiting >= 1:
                raw_size = self.ser.read(1)[0]
                size_byte = raw_size + 1
                if size_byte < 1 or size_byte > 63:
                    self.ser.reset_input_buffer()
                    continue
                
                payload = self.ser.read(size_byte)
                if len(payload) < size_byte:
                    self.ser.reset_input_buffer()
                    continue

                if payload[0] == 0x3A and len(payload) >= 4:
                    data_bytes = payload[1:]
                    if self.channel_mode == "Current & Voltage":
                        num_pairs = int(size_byte / 6)
                        for k in range(num_pairs):
                            v_raw = int.from_bytes(data_bytes[k*6 : k*6+3], byteorder='little')
                            i_raw = int.from_bytes(data_bytes[k*6+3 : k*6+6], byteorder='little')
                            v_adc, i_adc = v_raw >> 4, i_raw >> 4
                            if i_adc >= 524288: i_adc -= 1048576
                            v_batch.append((v_adc * self.v_mult) + self.v_offset)
                            i_batch.append((i_adc * self.i_mult) + self.i_offset)
                            t_batch.append(time.time() - self.start_time)
                            sample_index += 1
                    elif self.channel_mode == "Voltage":
                        for k in range(int(size_byte / 3)):
                            v_adc = int.from_bytes(data_bytes[k*3 : k*3+3], byteorder='little') >> 4
                            v_batch.append((v_adc * self.v_mult) + self.v_offset)
                            i_batch.append(0.0)
                            t_batch.append(time.time() - self.start_time)
                            sample_index += 1
                    elif self.channel_mode == "Current":
                        for k in range(int(size_byte / 3)):
                            i_adc = int.from_bytes(data_bytes[k*3 : k*3+3], byteorder='little') >> 4
                            if i_adc >= 524288: i_adc -= 1048576
                            v_batch.append(0.0)
                            i_batch.append((i_adc * self.i_mult) + self.i_offset)
                            t_batch.append(time.time() - self.start_time)
                            sample_index += 1

                    if len(v_batch) >= self.batch_size:
                        self.batch_received.emit(
                            np.array(v_batch, dtype=np.float32),
                            np.array(i_batch, dtype=np.float32),
                            np.array(t_batch, dtype=np.float32),
                            self.channel_mode
                        )
                        v_batch.clear(); i_batch.clear(); t_batch.clear()

                    ui_update_counter += 1
                    if ui_update_counter >= 100:
                        ui_update_counter = 0
                        elapsed = time.time() - self.start_time
                        self.status_changed.emit(f"Сэмплов: {sample_index}, Время: {elapsed:.3f} s")


@register
@styled_widget(N_('PP-INA228-USB Device'))
class CustomAdcWidget(QtWidgets.QWidget):
    CAPABILITIES = ['widget@']

    def __init__(self, parent=None):
        super().__init__(parent)
        
        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(10)
        
        title = QtWidgets.QLabel("PP-INA228-USB Device")
        title.setStyleSheet("font-size: 16px; font-weight: bold;")
        layout.addWidget(title)
        
        self.status_label = QtWidgets.QLabel("Статус: Остановлено")
        self.status_label.setStyleSheet("font-weight: bold; padding: 5px;")
        layout.addWidget(self.status_label)

        # --- Настройки ---
        settings_group = QtWidgets.QGroupBox("Настройки и калибровка")
        settings_layout = QtWidgets.QFormLayout(settings_group)
        
        self.port_input = QtWidgets.QLineEdit("COM13")
        settings_layout.addRow("COM port:", self.port_input)
        
        self.channel_combo = QtWidgets.QComboBox()
        self.channel_combo.addItems(["Current & Voltage", "Voltage", "Current"])
        settings_layout.addRow("Режим каналов:", self.channel_combo)
        
        self.sr_combo = QtWidgets.QComboBox()
        self.sr_combo.addItems(["20000", "11904.7619", "6666.6667", "3571.4285", "1851.85", "950.57", "482.16", "242.718"])
        settings_layout.addRow("Sample Rate (sps):", self.sr_combo)
        
        # НОВЫЕ ГАЛОЧКИ КОНФИГУРАЦИИ
        self.show_graphs_cb = QtWidgets.QCheckBox("Показывать графики в реальном времени (выкл. для экономии CPU)")
        settings_layout.addRow(self.show_graphs_cb)
        
        self.auto_open_cb = QtWidgets.QCheckBox("Автооткрытие файла в Waveform после записи")
        settings_layout.addRow(self.auto_open_cb)
        
        v_cal_layout = QtWidgets.QHBoxLayout()
        self.v_mult_input = QtWidgets.QLineEdit()
        self.v_mult_input.setValidator(QtGui.QDoubleValidator(-1e10, 1e10, 15, self))
        self.v_offset_input = QtWidgets.QLineEdit()
        self.v_offset_input.setValidator(QtGui.QDoubleValidator(-1e10, 1e10, 15, self))
        v_cal_layout.addWidget(QtWidgets.QLabel("Множитель:")); v_cal_layout.addWidget(self.v_mult_input)
        v_cal_layout.addWidget(QtWidgets.QLabel("Смещение:")); v_cal_layout.addWidget(self.v_offset_input)
        settings_layout.addRow("Voltage (V):", v_cal_layout)

        i_cal_layout = QtWidgets.QHBoxLayout()
        self.i_mult_input = QtWidgets.QLineEdit()
        self.i_mult_input.setValidator(QtGui.QDoubleValidator(-1e10, 1e10, 15, self))
        self.i_offset_input = QtWidgets.QLineEdit()
        self.i_offset_input.setValidator(QtGui.QDoubleValidator(-1e10, 1e10, 15, self))
        i_cal_layout.addWidget(QtWidgets.QLabel("Множитель:")); i_cal_layout.addWidget(self.i_mult_input)
        i_cal_layout.addWidget(QtWidgets.QLabel("Смещение:")); i_cal_layout.addWidget(self.i_offset_input)
        settings_layout.addRow("Current (A):", i_cal_layout)
        
        self.save_cal_btn = QtWidgets.QPushButton("💾 Сохранить настройки в файл")
        self.save_cal_btn.clicked.connect(self._save_calibration_ui)
        settings_layout.addRow("", self.save_cal_btn)
        layout.addWidget(settings_group)

        # --- ГРАФИКИ (создаются только если включена галочка) ---
        self.show_graphs = True # Значение будет перезаписано из конфига
        self.plot_v = None
        self.plot_i = None
        
        if PYQTGRAPH_AVAILABLE:
            self.v_axis = PrefixAxisItem('V', orientation='left')
            self.plot_v = pg.PlotWidget(title="Voltage", axisItems={'left': self.v_axis})
            self.plot_v.setLabel('bottom', 'Time', units='s')
            self.plot_v.showGrid(x=True, y=True, alpha=0.3)
            self.plot_v.setFixedHeight(120)
            self.curve_v = self.plot_v.plot(name='Voltage', pen=pg.mkPen('b', width=2))
            self.curve_v.setDownsampling(auto=True, method='peak')
            self.curve_v.setClipToView(True)
            # Авто-масштабирование по Y
            self.plot_v.enableAutoRange(axis=pg.ViewBox.YAxis, enable=True)
            layout.addWidget(self.plot_v)

            self.i_axis = PrefixAxisItem('A', orientation='left')
            self.plot_i = pg.PlotWidget(title="Current", axisItems={'left': self.i_axis})
            self.plot_i.setLabel('bottom', 'Time', units='s')
            self.plot_i.showGrid(x=True, y=True, alpha=0.3)
            self.plot_i.setFixedHeight(120)
            self.plot_i.setXLink(self.plot_v)
            self.curve_i = self.plot_i.plot(name='Current', pen=pg.mkPen('r', width=2))
            self.curve_i.setDownsampling(auto=True, method='peak')
            self.curve_i.setClipToView(True)
            # Авто-масштабирование по Y
            self.plot_i.enableAutoRange(axis=pg.ViewBox.YAxis, enable=True)
            layout.addWidget(self.plot_i)
            
            self.plot_v.addLegend(offset=(10, 10))
            self.plot_i.addLegend(offset=(10, 10))

        # --- Текстовый вывод ---
        data_frame = QtWidgets.QFrame()
        data_frame.setStyleSheet("border: 1px solid gray; border-radius: 5px; padding: 10px;")
        data_layout = QtWidgets.QHBoxLayout(data_frame)
        self.v_label = QtWidgets.QLabel("Voltage: ---- V")
        self.v_label.setStyleSheet("font-size: 20px; font-weight: bold;")
        self.i_label = QtWidgets.QLabel("Current: ---- A")
        self.i_label.setStyleSheet("font-size: 20px; font-weight: bold;")
        data_layout.addWidget(self.v_label); data_layout.addWidget(self.i_label)
        layout.addWidget(data_frame)

        # --- JLS ---
        if PYJLS_AVAILABLE:
            record_frame = QtWidgets.QFrame()
            record_frame.setStyleSheet("border: 1px solid gray; border-radius: 5px; padding: 10px;")
            record_layout = QtWidgets.QVBoxLayout(record_frame)
            record_layout.addWidget(QtWidgets.QLabel("Запись в JLS (Joulescope Format)"))
            path_layout = QtWidgets.QHBoxLayout()
            self.path_input = QtWidgets.QLineEdit("recording.jls")
            self.browse_btn = QtWidgets.QPushButton("Обзор...")
            self.browse_btn.clicked.connect(self._browse_file)
            path_layout.addWidget(self.path_input); path_layout.addWidget(self.browse_btn)
            record_layout.addLayout(path_layout)
            self.record_label = QtWidgets.QLabel("Запись: неактивна")
            self.record_label.setWordWrap(True)
            record_layout.addWidget(self.record_label)
            layout.addWidget(record_frame)

        # --- Кнопки ---
        btn_layout = QtWidgets.QHBoxLayout()
        self.start_btn = QtWidgets.QPushButton("▶ Старт")
        self.start_btn.setStyleSheet("font-weight: bold; padding: 8px;")
        self.stop_btn = QtWidgets.QPushButton("■ Стоп")
        self.stop_btn.setStyleSheet("font-weight: bold; padding: 8px;")
        self.stop_btn.setEnabled(False)
        btn_layout.addWidget(self.start_btn); btn_layout.addWidget(self.stop_btn)
        layout.addLayout(btn_layout)

        # --- Поток и переменные ---
        self.thread = QtCore.QThread()
        self.worker = SerialWorker()
        self.worker.moveToThread(self.thread)

        self.start_btn.clicked.connect(self._on_start)
        self.stop_btn.clicked.connect(self._on_stop)
        
        if PYQTGRAPH_AVAILABLE:
            self.worker.batch_received.connect(self._on_batch_received)
        self.worker.status_changed.connect(self.status_label.setText)
        self.thread.started.connect(self.worker.start_device)
        
        self.jls_writer = None
        self.v_signal_id = 1
        self.i_signal_id = 2
        self.total_samples = 0
        
        self.v_buffer, self.i_buffer = [], []
        self.current_mode = "Current & Voltage"
        
        if PYQTGRAPH_AVAILABLE:
            self.plot_time = collections.deque(maxlen=2000)
            self.plot_v_data = collections.deque(maxlen=2000)
            self.plot_i_data = collections.deque(maxlen=2000)
            self.plot_start_time = None
        
        self.update_timer = QtCore.QTimer()
        self.update_timer.timeout.connect(self._update_ui_from_buffer)
        self.update_timer.start(1000)
        
        self._load_calibration_ui()

    def _load_calibration_ui(self):
        config = load_config()
        self.port_input.setText(config.get("com_port", "COM13"))
        self.v_mult_input.setText(f"{config['v_mult']:.10e}")
        self.v_offset_input.setText(f"{config['v_offset']:.6f}")
        self.i_mult_input.setText(f"{config['i_mult']:.10e}")
        self.i_offset_input.setText(f"{config['i_offset']:.6f}")
        
        idx = self.sr_combo.findText(str(config["sample_rate"]))
        if idx >= 0: self.sr_combo.setCurrentIndex(idx)
        idx = self.channel_combo.findText(config["channel_mode"])
        if idx >= 0: self.channel_combo.setCurrentIndex(idx)
        
        # Загрузка новых настроек
        self.show_graphs = config.get("show_graphs", True)
        self.show_graphs_cb.setChecked(self.show_graphs)
        
        self.auto_open_waveform = config.get("auto_open_waveform", False)
        self.auto_open_cb.setChecked(self.auto_open_waveform)
        
        # Применение настройки графиков
        if self.plot_v:
            self.plot_v.setVisible(self.show_graphs)
        if self.plot_i:
            self.plot_i.setVisible(self.show_graphs)

    def _save_calibration_ui(self):
        try:
            config = {
                "com_port": self.port_input.text(),
                "v_mult": float(self.v_mult_input.text()),
                "v_offset": float(self.v_offset_input.text()),
                "i_mult": float(self.i_mult_input.text()),
                "i_offset": float(self.i_offset_input.text()),
                "sample_rate": float(self.sr_combo.currentText()),
                "channel_mode": self.channel_combo.currentText(),
                "show_graphs": self.show_graphs_cb.isChecked(),
                "auto_open_waveform": self.auto_open_cb.isChecked()
            }
            save_config(config)
            self.status_label.setText("✅ Настройки сохранены в файл")
            QtCore.QTimer.singleShot(2000, lambda: self.status_label.setText("Статус: Остановлено"))
        except ValueError:
            self.status_label.setText("❌ Ошибка: некорректное числовое значение")

    def _browse_file(self):
        file_path, _ = QtWidgets.QFileDialog.getSaveFileName(self, "Сохранить JLS", "", "JLS Files (*.jls)")
        if file_path: self.path_input.setText(file_path)

    def _start_jls(self):
        if not PYJLS_AVAILABLE: return
        file_path = self.path_input.text() or "recording.jls"
        try:
            self.jls_writer = Writer(file_path)
            self.jls_writer.source_def(source_id=1, name="PP INA228 USB Device", vendor="MyLab", model="INA2282USB", serial_number="000001")
            
            sr = float(self.sr_combo.currentText())
            # уточним значение sample rate в целых интервалах мкс
            sr = int(round(1000000.0 / sr))
            # получим точное значение в sps
            sr = 1000000.0 / sr

            mode = self.channel_combo.currentText()
            self.jls_sr = (sr / 2.0) if mode == "Current & Voltage" else float(sr)
            
            EPOCH_2018 = 1514764800.0
            SCALE = 1 << 30
            utc_start = int((time.time() - EPOCH_2018) * SCALE)
            
            if mode in ["Current & Voltage", "Voltage"]:
                self.jls_writer.signal_def(signal_id=1, source_id=1, signal_type=SignalType.FSR, data_type=DataType.F32, sample_rate=self.jls_sr, name="voltage", units="V")
                self.jls_writer.utc(1, 0, utc_start)
            if mode in ["Current & Voltage", "Current"]:
                self.jls_writer.signal_def(signal_id=2, source_id=1, signal_type=SignalType.FSR, data_type=DataType.F32, sample_rate=self.jls_sr, name="current", units="A")
                self.jls_writer.utc(2, 0, utc_start)
            
            self.record_label.setText("Запись: активна")
            self.total_samples = 0
        except Exception as e:
            self.record_label.setText(f"Ошибка записи JLS:\n{str(e)}")
            self.jls_writer = None    
    
    def _stop_jls(self):    
        if self.jls_writer:
            try:
                self.jls_writer.close()
                
                tsp = 1.0 / self.jls_sr
                
                self.record_label.setText(f"Запись завершена.\nСохранено сэмплов: {self.total_samples}, длительность: {self.total_samples * tsp:.3f} s")
                
                # Автооткрытие в Waveform
                if self.auto_open_waveform:
                    # Получаем абсолютный путь для надежности
                    file_path = os.path.abspath(self.path_input.text())
                    if os.path.exists(file_path):
                        try:
                            # ПРАВИЛЬНЫЙ топик для открытия файла в главном окне Joulescope UI
                            self.pubsub.publish('registry/ui/actions/!file_open', file_path)
                            self.status_label.setText("✅ Файл открыт в Waveform")
                        except Exception as e:
                            print(f"Не удалось открыть файл в Waveform: {e}")
                            self.status_label.setText("⚠ Ошибка открытия в Waveform")
                            
            except Exception as e:
                self.record_label.setText(f"Ошибка закрытия: {e}")
            finally:
                self.jls_writer = None

    def _on_batch_received(self, v_array, i_array, t_array, mode):
        self.current_mode = mode
        
        # 1. Запись в JLS (всегда работает, это быстро)
        if self.jls_writer:
            try:
                sample_offset = self.total_samples
                if mode in ["Current & Voltage", "Voltage"]:
                    self.jls_writer.fsr_f32(self.v_signal_id, sample_offset, v_array)
                if mode in ["Current & Voltage", "Current"]:
                    self.jls_writer.fsr_f32(self.i_signal_id, sample_offset, i_array)
                self.total_samples += len(v_array)
                if self.total_samples % 5000 < len(v_array):
                    self.jls_writer.flush()
            except Exception as e:
                print(f"JLS write error: {e}")
        
        # 2. Обновление графиков ТОЛЬКО если включено в конфигурации
        if self.show_graphs and self.plot_v:
            if self.plot_start_time is None:
                self.plot_start_time = time.time()
            
            self.plot_time.extend(t_array.tolist())
            self.plot_v_data.extend(v_array.tolist())
            self.plot_i_data.extend(i_array.tolist())
            
            # Для текста используем быстрое усреднение numpy
            self.v_buffer.append(np.mean(v_array))
            self.i_buffer.append(np.mean(i_array))
    
    def _update_ui_from_buffer(self):
        # Если графики отключены, мы все равно обновляем текст, но не трогаем тяжелые массивы графиков
        if not self.v_buffer and not self.i_buffer:
            return
        
        v_avg = float(np.mean(self.v_buffer)) if self.v_buffer else 0.0
        i_avg = float(np.mean(self.i_buffer)) if self.i_buffer else 0.0
        
        self.v_buffer.clear()
        self.i_buffer.clear()
        
        v_str = format_with_prefix(v_avg, 'V')
        i_str = format_with_prefix(i_avg, 'A')
        
        if self.current_mode == "Voltage":
            self.v_label.setText(f"Voltage: {v_str}")
            self.i_label.setText("Current: N/A")
        elif self.current_mode == "Current":
            self.v_label.setText("Voltage: N/A")
            self.i_label.setText(f"Current: {i_str}")
        else:
            self.v_label.setText(f"Voltage: {v_str}")
            self.i_label.setText(f"Current: {i_str}")

        # Обновление данных на графиках только если они включены
        if self.show_graphs and self.plot_v and self.plot_time:
            t_arr = np.array(self.plot_time, dtype=np.float32)
            v_arr = np.array(self.plot_v_data, dtype=np.float32)
            i_arr = np.array(self.plot_i_data, dtype=np.float32)
            self.curve_v.setData(t_arr, v_arr)
            self.curve_i.setData(t_arr, i_arr)

    def _on_start(self):
        try:
            # Читаем актуальные значения настроек
            self.show_graphs = self.show_graphs_cb.isChecked()
            self.auto_open_waveform = self.auto_open_cb.isChecked()
            
            self.worker.port = self.port_input.text()
            self.worker.v_mult = float(self.v_mult_input.text())
            self.worker.v_offset = float(self.v_offset_input.text())
            self.worker.i_mult = float(self.i_mult_input.text())
            self.worker.i_offset = float(self.i_offset_input.text())
            self.worker.sample_rate = float(self.sr_combo.currentText())
            self.worker.channel_mode = self.channel_combo.currentText()
            
            sr = self.worker.sample_rate
            self.worker.batch_size = max(100, min(5000, sr // 5))
        except ValueError:
            self.status_label.setText("❌ Ошибка: некорректные калибровочные значения")
            return
        
        self.start_btn.setEnabled(False)
        self.stop_btn.setEnabled(True)
        self.port_input.setEnabled(False)
        self.sr_combo.setEnabled(False)
        self.channel_combo.setEnabled(False)
        self.show_graphs_cb.setEnabled(False) # Блокируем изменение во время записи
        
        if self.plot_v:
            self.plot_v.setVisible(self.show_graphs)
            self.plot_i.setVisible(self.show_graphs)
            self.plot_time.clear()
            self.plot_v_data.clear()
            self.plot_i_data.clear()
            self.plot_start_time = None
        
        self._start_jls()
        self.thread.start()

    def _on_stop(self):
        self.worker.stop_device()
        self._stop_jls()
        
        self.start_btn.setEnabled(True)
        self.stop_btn.setEnabled(False)
        self.port_input.setEnabled(True)
        self.sr_combo.setEnabled(True)
        self.channel_combo.setEnabled(True)
        self.show_graphs_cb.setEnabled(True)
        
        self.thread.quit()
        self.thread.wait()

    def on_pubsub_register(self): pass

    def mousePressEvent(self, event):
        event.accept()
        if event.button() == QtCore.Qt.RightButton:
            menu = QtWidgets.QMenu(self)
            settings_action_create(self, menu)
            context_menu_show(menu, event)

    def closeEvent(self, event):
        self.update_timer.stop()
        self._on_stop()
        super().closeEvent(event)