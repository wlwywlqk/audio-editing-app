import { useState, useCallback, useEffect, useRef } from 'react';
import './index.scss';

const Module = window.Module;

function Form() {
  const [info, setInfo] = useState(null);
  const $form = useRef(null);

  const onSubmit = useCallback((e) => {
    e.preventDefault();

    const fileReader = new FileReader();
    fileReader.onload = function() {
      const buffer = new Uint8Array(this.result);
      const bufferP = Module._malloc(buffer.length);

      Module.HEAP8.set(buffer, bufferP);
      Module._print_metadata(bufferP, buffer.length);
      Module._free(bufferP);
    }
    const file = $form.current.file.files[0];
    fileReader.readAsArrayBuffer(file);


  }, []);
  return <>
    <form ref={$form} className="form" onSubmit={onSubmit} >
      <input type="file" name="file" required />
      <input type="submit" value="获取文件信息" />
    </form>
    {info && <pre>{JSON.stringify(info, null, 4)}</pre>}
  </>;
}

export default Form;
